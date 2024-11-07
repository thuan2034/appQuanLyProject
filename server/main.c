#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "client_handler.h"
#include "session.h"

#define PORT 5000

int main() {
    SessionManager session_manager;
    init_session_manager(&session_manager);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("Server listening on port %d\n", PORT);

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        printf("Client connected\n");
        pthread_t thread_id;
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        ClientHandlerArgs *client_args = malloc(sizeof(ClientHandlerArgs));
        client_args->session_manager = &session_manager;
        client_args->socket = new_socket;
        pthread_create(&thread_id, NULL, client_handler, (void *)client_args);
    }

    close(server_fd);
    return 0;
}
