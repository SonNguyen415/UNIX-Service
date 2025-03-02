#include "domain_sockets.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 10
#define MAX_CLIENTS 100

int client_sockets[MAX_CLIENTS];
int num_clients = 0;

void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void broadcast_message(struct chat_message *msg, int sender_fd) {
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] != sender_fd && client_sockets[i] > 0) {
            write(client_sockets[i], msg, sizeof(struct chat_message));
        }
    }
}

void add_client(int client_fd) {
    if (num_clients < MAX_CLIENTS) {
        client_sockets[num_clients++] = client_fd;
    }
}

void remove_client(int client_fd) {
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] == client_fd) {
            client_sockets[i] = client_sockets[num_clients - 1];
            num_clients--;
            break;
        }
    }
}

int main(void) {
    char *ds = "chat_socket";
    int socket_desc, epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];

    // Initialize client array
    memset(client_sockets, 0, sizeof(client_sockets));

    socket_desc = domain_socket_server_create(ds);
    if (socket_desc < 0) {
        /* Remove the previous domain socket file if it exists */
        unlink(ds);
        socket_desc = domain_socket_server_create(ds);
        if (socket_desc < 0) panic("server domain socket creation");
    }

    printf("Chat server is running...\n");

    /* Handle multiple clients sequentially */
    /* TODO: Liza change this to handle clients concurrently*/
    set_nonblocking(socket_desc);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) panic("epoll_create1");

    event.data.fd = socket_desc;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_desc, &event) == -1) panic("epoll_ctl");

    // notes from liza: used chatGPT to aid with event loop creation
    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) panic("epoll_wait");

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == socket_desc) {
                // New connection
                int client_fd = accept(socket_desc, NULL, NULL);
                if (client_fd == -1) continue;
                
                set_nonblocking(client_fd);
                event.data.fd = client_fd;
                event.events = EPOLLIN;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                add_client(client_fd);
                
                printf("New client connected!\n");
            } else {
                // Existing client sent data
                int client_fd = events[i].data.fd;
                struct chat_message msg;
                
                int bytes = read(client_fd, &msg, sizeof(msg));
                if (bytes <= 0) {
                    // Client disconnected
                    remove_client(client_fd);
                    close(client_fd);
                    printf("Client disconnected\n");
                } else {
                    // Broadcast message to all other clients
                    printf("%s: %s\n", msg.username, msg.content);
                    broadcast_message(&msg, client_fd);
                }
            }
        }
    }

    close(socket_desc);
    close(epoll_fd);
    unlink(ds);
    printf("Chat server shutting down...\n");

    return 0;
}
