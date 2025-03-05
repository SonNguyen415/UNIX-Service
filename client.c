#include "domain_sockets.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct dm_connection {
    char username[MAX_USERNAME];
    int fd;
};

#define MAX_DM_CONNECTIONS 10
struct dm_connection dm_connections[MAX_DM_CONNECTIONS];
int num_dm_connections = 0;

void add_dm_connection(const char *username, int fd) {
    if (num_dm_connections < MAX_DM_CONNECTIONS) {
        strncpy(dm_connections[num_dm_connections].username, username, MAX_USERNAME - 1);
        dm_connections[num_dm_connections].fd = fd;
        num_dm_connections++;
    }
}

int find_dm_fd(const char *username) {
    for (int i = 0; i < num_dm_connections; i++) {
        if (strcmp(dm_connections[i].username, username) == 0) {
            return dm_connections[i].fd;
        }
    }
    return -1;
}

void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void *receive_messages(void *socket_ptr) {
    int socket_desc = *(int *)socket_ptr;
    struct chat_message msg;

    while (1) {
        if (read(socket_desc, &msg, sizeof(msg)) > 0) {
            if (msg.is_dm == 2) {  // DM setup message
                printf("Receiving DM setup from %s\n", msg.username);
                int peer_fd = recv_fd(socket_desc);
                if (peer_fd < 0) {
                    perror("Failed to receive fd");
                    continue;
                }
                printf("Received fd: %d for DM with %s\n", peer_fd, msg.username);
                add_dm_connection(msg.username, peer_fd);
                printf("Established DM connection with %s\n", msg.username);
            } else if (msg.is_dm) {
                printf("[DM] %s: %s\n", msg.username, msg.content);
            } else {
                printf("%s: %s\n", msg.username, msg.content);
            }
        } else {
            break;  // Connection closed or error
        }
    }
    return NULL;
}

void parse_message(struct chat_message *msg, char *input) {
    msg->is_dm = 0;
    
    if (input[0] == '@') {
        char *space = strchr(input, ' ');
        if (space) {
            int username_len = space - input - 1;
            strncpy(msg->target, input + 1, username_len);
            msg->target[username_len] = '\0';
            strncpy(msg->content, space + 1, MAX_MSG_SIZE);
            msg->is_dm = 1;
            
            // Check if we already have a DM connection
            int dm_fd = find_dm_fd(msg->target);
            if (dm_fd != -1) {
                // Send directly to peer
                write(dm_fd, msg, sizeof(struct chat_message));
                return;
            }
        }
    }
    strncpy(msg->content, input, MAX_MSG_SIZE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int socket_desc = domain_socket_client_create("chat_socket");
    if (socket_desc < 0) panic("Failed to connect to chat server");

    // Create thread for receiving messages
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, &socket_desc) != 0) {
        panic("Failed to create receive thread");
    }

    // Main thread handles sending messages, send an empty message first to register
    struct chat_message msg;
    strncpy(msg.username, argv[1], MAX_USERNAME - 1);
    msg.is_dm = 0;
    if (write(socket_desc, &msg, sizeof(msg)) == -1) {
        panic("Failed to register with chat server");
    }

    printf("Connected to chat. Type your messages:\n");
    while (1) {
        char input[MAX_MSG_SIZE];
        if (fgets(input, MAX_MSG_SIZE, stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;  // Remove newline

        if (strcmp(input, "quit") == 0) break;

        parse_message(&msg, input);
        if (write(socket_desc, &msg, sizeof(msg)) == -1) {
            panic("Failed to send message");
        }
    }

    close(socket_desc);
    return 0;
}
