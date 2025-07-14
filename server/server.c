#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#include "../shared/protocol.h"
#include "handler.h"

int main() {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in adr = {0};
    adr.sin_family = AF_INET;
    adr.sin_port = htons(PORT);
    adr.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv, (struct sockaddr*)&adr, sizeof(adr)) < 0) {
        perror("bind");
        close(srv);
        exit(EXIT_FAILURE);
    }

    if (listen(srv, 10) < 0) {
        perror("listen");
        close(srv);
        exit(EXIT_FAILURE);
    }

    puts("🔊 Server ready...");

    while (1) {
        int* cli = malloc(sizeof(int));
        if (!cli) {
            perror("malloc");
            continue;
        }

        *cli = accept(srv, NULL, NULL);
        if (*cli < 0) {
            perror("accept");
            free(cli);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, cli) != 0) {
            perror("pthread_create");
            close(*cli);
            free(cli);
            continue;
        }

        pthread_detach(tid);
    }

    close(srv);
    return 0;
}

