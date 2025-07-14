#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <openssl/sha.h>

#include "../shared/protocol.h"
#include "handler.h"

/* ---------- user helpers ---------- */
static int user_exists(const char* u)
{
    FILE* fp = fopen("users.txt", "r");
    if (!fp) return 0;
    char line[128], name[MAX_USERNAME], pw[MAX_PASSWORD];
    while (fgets(line, sizeof(line), fp))
        if (sscanf(line, "%[^:]:%s", name, pw) == 2 && strcmp(name, u) == 0)
        { fclose(fp); return 1; }
    fclose(fp); return 0;
}
static int auth_ok(const char* u, const char* p)
{
    FILE* fp = fopen("users.txt", "r");
    if (!fp) return 0;
    char line[128], name[MAX_USERNAME], pw[MAX_PASSWORD];
    while (fgets(line, sizeof(line), fp))
        if (sscanf(line, "%[^:]:%s", name, pw) == 2 &&
            strcmp(name, u) == 0 && strcmp(pw, p) == 0)
        { fclose(fp); return 1; }
    fclose(fp); return 0;
}
/* ---------- hash helper ---------- */
static void hexify(unsigned char h[SHA256_DIGEST_LENGTH], char out[HASH_STR_LEN])
{
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) sprintf(out + i*2, "%02x", h[i]);
    out[64] = '\0';
}
/* ---------- thread entry ---------- */
void* client_handler(void* socket_desc)
{
    int sock = *(int*)socket_desc; free(socket_desc);

    char cmd[16] = {0}, user[MAX_USERNAME] = {0}, pass[MAX_PASSWORD] = {0};
    recv(sock, cmd,  sizeof(cmd),  0);
    recv(sock, user, sizeof(user), 0);
    recv(sock, pass, sizeof(pass), 0);

    /* -------- REGISTER -------- */
    if (strcasecmp(cmd,"REGISTER")==0) {
        if (user_exists(user)) { send(sock,"USER_EXISTS",12,0); close(sock); return NULL; }
        FILE* f=fopen("users.txt","a"); fprintf(f,"%s:%s\n",user,pass); fclose(f);
        send(sock,"AUTH_SUCCESS",13,0); close(sock); return NULL;
    }

    /* -------- AUTH -------- */
    if (!auth_ok(user,pass)) { send(sock,"AUTH_FAILED",12,0); close(sock); return NULL; }
    send(sock,"AUTH_SUCCESS",13,0);

    /* ===== UPLOAD ===== */
        if (strcasecmp(cmd,"UPLOAD")==0)
{
    char fname[MAX_FILENAME]={0}, hash_cli[HASH_STR_LEN]={0}, vis[8]={0};
    long fsize=0;
    recv(sock, fname, sizeof(fname), 0);
    recv(sock, &fsize, sizeof(long), 0);
    recv(sock, hash_cli, sizeof(hash_cli), 0);
    recv(sock, vis, sizeof(vis), 0);

    if (fsize <= 0) {
        close(sock);
        return NULL;
    }

    // Create user folder if not exists
    char user_dir[300];
    snprintf(user_dir, sizeof(user_dir), "uploads/%s", user);
    mkdir("uploads", 0777);
    mkdir(user_dir, 0777);

    // Build final path to store
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char stored_name[256];
    snprintf(stored_name, sizeof(stored_name), "%02d%02d%02d_%s",
             t->tm_hour, t->tm_min, t->tm_sec, strrchr(fname, '/') ? strrchr(fname, '/') + 1 : fname);

    char final_path[512];
    snprintf(final_path, sizeof(final_path), "%s/%s", user_dir, stored_name);

    FILE* fp = fopen(final_path, "wb");
    if (!fp) {
        perror("fopen");
        close(sock);
        return NULL;
    }

    // Receive file data and compute hash
    char buf[BUFFER_SIZE];
    long rec = 0;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    clock_t start = clock();
    while (rec < fsize) {
        int n = recv(sock, buf, BUFFER_SIZE, 0);
        if (n <= 0) break;
        fwrite(buf, 1, n, fp);
        SHA256_Update(&ctx, buf, n);
        rec += n;
    }
    fclose(fp);
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    double speed_kbps = rec / 1024.0 / (seconds ? seconds : 1.0);

    unsigned char h[SHA256_DIGEST_LENGTH];
    SHA256_Final(h, &ctx);
    char hash_srv[HASH_STR_LEN];
    hexify(h, hash_srv);
    int ok = strcmp(hash_cli, hash_srv) == 0;

    // Save index
    FILE* meta = fopen("file_index.txt", "a");
    if (meta) {
        fprintf(meta, "%s:%s:%s\n", user, stored_name, vis);
        fclose(meta);
    }

    // Save log
    FILE* log = fopen("logs.txt", "a");
    if (log) {
        fprintf(log, "[UPLOAD] %s uploaded %s (%ldB) %.2fs %.2fKB/s [%s]\n",
                user, final_path, rec, seconds, speed_kbps, ok ? "OK" : "HASH!");
        fclose(log);
    }

    // Send stored file path back to client
    send(sock, stored_name, sizeof(stored_name), 0);
    printf("⬆ %s uploaded %s (%ldB) %.2fs %.2fKB/s [%s]\n",
           user, final_path, rec, seconds, speed_kbps, ok ? "OK" : "HASH!");

    close(sock);
    return NULL;
}

        

        

    /* ===== DOWNLOAD ===== */
    /* ===== DOWNLOAD ===== */
if (strcasecmp(cmd,"DOWNLOAD")==0)
{
    char req[MAX_FILENAME]={0}; 
    recv(sock, req, sizeof(req), 0);

    int allowed = 0;
    char owner[MAX_USERNAME] = {0};
    char acc[8] = {0};

    // Search for file and permissions
    FILE* meta = fopen("file_index.txt", "r");
    if (meta) {
        char line[512], fn[MAX_FILENAME];
        while (fgets(line, sizeof(line), meta)) {
            if (sscanf(line, "%[^:]:%[^:]:%s", owner, fn, acc) == 3 && strcmp(fn, req) == 0) {
                if (strcmp(owner, user) == 0 || strcmp(acc, "public") == 0)
                    allowed = 1;
                break;
            }
        }
        fclose(meta);
    }

    if (!allowed) {
        long d = -2; // Access denied
        send(sock, &d, sizeof(long), 0);
        close(sock); return NULL;
    }

    char path[300];
    snprintf(path, sizeof(path), "uploads/%s/%s", owner, req);

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        long nf = -1; // File not found
        send(sock, &nf, sizeof(long), 0);
        close(sock); return NULL;
    }

    fseek(fp, 0, SEEK_END); 
    long size = ftell(fp); 
    rewind(fp);

    SHA256_CTX ctx; 
    SHA256_Init(&ctx);
    char buf[BUFFER_SIZE]; 
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        SHA256_Update(&ctx, buf, n);
    
    unsigned char h[SHA256_DIGEST_LENGTH]; 
    SHA256_Final(h, &ctx);
    char hash[HASH_STR_LEN]; 
    hexify(h, hash); 
    rewind(fp);

    // Send file size and hash
    send(sock, &size, sizeof(long), 0); 
    send(sock, hash, sizeof(hash), 0);

    // Send file data
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        send(sock, buf, n, 0);
    fclose(fp); 
    close(sock);

    // Log
    FILE* log = fopen("logs.txt", "a");
    if (log) {
        fprintf(log, "[DOWNLOAD] %s downloaded %s (%ldB)\n", user, path, size);
        fclose(log);
    }
    printf("⬇ %s downloaded %s (%ldB)\n", user, path, size);
    return NULL;
}


    close(sock);
    return NULL;
}

