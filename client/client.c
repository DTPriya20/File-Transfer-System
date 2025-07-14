// client/client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include<sys/stat.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include "../shared/protocol.h"

/* ---------- SHA256 file hash ---------- */
static void file_sha256(FILE* fp, char out[HASH_STR_LEN]) {
    unsigned char buf[BUFFER_SIZE], bin[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        EVP_DigestUpdate(ctx, buf, n);
    EVP_DigestFinal_ex(ctx, bin, &hash_len);
    for (int i = 0; i < hash_len; i++)
        sprintf(out + i * 2, "%02x", bin[i]);
    out[64] = '\0';
    rewind(fp);
    EVP_MD_CTX_free(ctx);
}

/* ---------- hide password input ---------- */
void secure_input(char* buffer, size_t size) {
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old; new.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    fgets(buffer, size, stdin);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    buffer[strcspn(buffer, "\n")] = '\0';
}

int main() {
    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in srv = {0};
        srv.sin_family = AF_INET;
        srv.sin_port = htons(PORT);
        srv.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
            perror("connect"); return 1;
        }

        char cmd[16];
        printf("\n📦 Command Options: REGISTER / UPLOAD / DOWNLOAD / EXIT\nCommand: ");
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcasecmp(cmd, "EXIT") == 0) {
            close(sock);
            puts("👋 Bye!"); break;
        }

        send(sock, cmd, sizeof(cmd), 0);

        char user[MAX_USERNAME], pass[MAX_PASSWORD];
        printf("Username: "); fgets(user, sizeof(user), stdin);
        user[strcspn(user, "\n")] = '\0';
        printf("Password: "); secure_input(pass, sizeof(pass));

        send(sock, user, sizeof(user), 0);
        send(sock, pass, sizeof(pass), 0);

        char auth[32] = {0};
        recv(sock, auth, sizeof(auth), 0);
        if (strcmp(auth, "AUTH_FAILED") == 0) {
            printf("❌ Server: %s\n", auth); close(sock); continue;
        }

        if (strcasecmp(cmd, "REGISTER") == 0) {
            puts("✅ Registered."); close(sock); continue;
        }

       //upload updated
           
        if (strcasecmp(cmd, "UPLOAD") == 0) {
    char fname[MAX_FILENAME];
    printf("File to upload: ");
    fgets(fname, sizeof(fname), stdin);
    fname[strcspn(fname, "\n")] = '\0'; // Remove newline

    // Open file for reading
    FILE* fp = fopen(fname, "rb");
    if (!fp) {
        perror("❌ fopen failed");
        close(sock);
        continue;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    // Check if file is empty
    if (fsize <= 0) {
        printf("❌ File is empty or unreadable. Upload aborted.\n");
        fclose(fp);
        close(sock);
        continue;
    }

    // Compute SHA256 hash
    char hash[HASH_STR_LEN];
    file_sha256(fp, hash);

    // Send metadata
    send(sock, fname, sizeof(fname), 0);
    send(sock, &fsize, sizeof(long), 0);
    send(sock, hash, sizeof(hash), 0);

    // Send visibility
    char vis[8];
    printf("public or private? ");
    fgets(vis, sizeof(vis), stdin);
    vis[strcspn(vis, "\n")] = '\0';
    send(sock, vis, sizeof(vis), 0);

    // Send file content
    char buf[BUFFER_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        send(sock, buf, n, 0);
    fclose(fp);

    // Receive server's saved file path
    char stored_path[300] = {0};
    recv(sock, stored_path, sizeof(stored_path), 0);
    if (strlen(stored_path) > 0)
        printf("⬆ Upload done.\n📍 Stored as: uploads/%s\n", stored_path);
    else
        printf("❌ Upload failed.\n");

    close(sock);
}

    

    
//download

        else if (strcasecmp(cmd, "DOWNLOAD") == 0) {
            char fname[MAX_FILENAME];
            printf("File to download: "); fgets(fname, sizeof(fname), stdin);
            fname[strcspn(fname, "\n")] = '\0';
            send(sock, fname, sizeof(fname), 0);

            long fsize;
            recv(sock, &fsize, sizeof(long), 0);
            if (fsize == -1) { puts("❌ File not found."); close(sock); continue; }
            if (fsize == -2) { puts("❌ Access denied."); close(sock); continue; }

            char hash_srv[HASH_STR_LEN];
            recv(sock, hash_srv, sizeof(hash_srv), 0);

            char user_dir[100];
            snprintf(user_dir, sizeof(user_dir), "%s", user);
            mkdir(user_dir, 0777);  // Create the user directory if not exists

            char out[300];
            snprintf(out, sizeof(out), "%s/%s", user_dir, fname);
            FILE* fp = fopen(out, "wb");
            if (!fp) { perror("fopen"); close(sock); continue; }

            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
            char buf[BUFFER_SIZE];
            long rec = 0;
            while (rec < fsize) {
                int n = recv(sock, buf, BUFFER_SIZE, 0);
                if (n <= 0) break;
                fwrite(buf, 1, n, fp);
                EVP_DigestUpdate(mdctx, buf, n);
                rec += n;
            }

            fclose(fp); close(sock);
            unsigned char bin[EVP_MAX_MD_SIZE];
            unsigned int hash_len;
            EVP_DigestFinal_ex(mdctx, bin, &hash_len);
            EVP_MD_CTX_free(mdctx);

            char hash_cli[HASH_STR_LEN];
            for (int i = 0; i < hash_len; i++)
                sprintf(hash_cli + i * 2, "%02x", bin[i]);
            hash_cli[64] = '\0';

            printf("⬇ Downloaded %ldB as %s\nIntegrity: %s\n", rec, out,
                   strcmp(hash_cli, hash_srv) == 0 ? "✅ OK" : "❌ MISMATCH");
        }

        else {
            puts("❓ Unknown command.");
        }
    }
    return 0;
}

