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
#define MAX_CLIENTS 10

struct user_info {
    char username[MAX_USERNAME];
    int socket_fd;
};

struct user_info users[MAX_CLIENTS];

int num_clients = 0;

void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void add_client(int client_fd, const char *username) {
    if (num_clients < MAX_CLIENTS) {
        strncpy(users[num_clients].username, username, MAX_USERNAME - 1);
        users[num_clients].socket_fd = client_fd;
        num_clients++;
    }
}

void remove_client(int client_fd) {
    for (int i = 0; i < num_clients; i++) {
        if (users[i].socket_fd == client_fd) {
            users[i] = users[num_clients - 1];
            num_clients--;
            break;
        }
    }
}

int find_user_socket(const char *username) {
    for (int i = 0; i < num_clients; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return users[i].socket_fd;
        }
    }
    return -1;
}

void handle_message(struct chat_message *msg, int sender_fd) {
    // If this is the first message from a client, update their username
    for (int i = 0; i < num_clients; i++) {
        if (users[i].socket_fd == sender_fd && users[i].username[0] == '\0') {
            strncpy(users[i].username, msg->username, MAX_USERNAME - 1);
            printf("User %s registered with fd %d\n", msg->username, sender_fd);
            break;
    }

    if (msg->is_dm) {
        int target_fd = find_user_socket(msg->target);
        if (target_fd != -1) {
            printf("Setting up DM between %s and %s\n", msg->username, msg->target);
            
            // Send target's fd to sender
            if (send_fd(sender_fd, target_fd) < 0) {
                perror("Failed to send fd to sender");
                return;
            }
            
            // Send sender's fd to target
            struct chat_message setup_msg = {0};
            setup_msg.is_dm = 2;  // Special flag for DM setup
            strncpy(setup_msg.username, msg->username, MAX_USERNAME - 1);
            write(target_fd, &setup_msg, sizeof(setup_msg));
            if (send_fd(target_fd, sender_fd) < 0) {
                perror("Failed to send fd to target");
                return;
            }
            
            // Send the original message through the server this first time
            write(target_fd, msg, sizeof(struct chat_message));
            printf("DM setup complete\n");
        } else {
            printf("Target user %s not found\n", msg->target);
        }
    } else {
        // Regular broadcast
        printf("Broadcasting message from %s\n", msg->username);
        for (int i = 0; i < num_clients; i++) {
            if (users[i].socket_fd != sender_fd) {
                write(users[i].socket_fd, msg, sizeof(struct chat_message));
            }
        }
    }
}

int main(void) {
    char *ds = "chat_socket";
    int socket_desc, epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];

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
                add_client(client_fd, "");
                
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
                    handle_message(&msg, client_fd);
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
