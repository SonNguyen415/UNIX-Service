#include "domain_sockets.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <logging.h>
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
            if (msg.is_dm) {
                log_message("[DM] %s: %s\n", msg.username, msg.content);
            } else {
                log_message("%s: %s\n", msg.username, msg.content);
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
        if (space && (space - input) > 1) {  // Ensure there's a username
            int username_len = space - input - 1;
            strncpy(msg->target, input + 1, username_len);
            msg->target[username_len] = '\0';
            strncpy(msg->content, space + 1, MAX_MSG_SIZE - 1);
            msg->is_dm = 1;
        } else {
            strncpy(msg->content, "Invalid DM format. Use: @username message", MAX_MSG_SIZE - 1);
        }
    } else {
        strncpy(msg->content, input, MAX_MSG_SIZE - 1);
    }
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
    if (register_user(socket_desc, &msg) == -1) {
        panic("Failed to register user");
    }
    
    // Create output file
    create_file(argv[1]);

    log_message("Connected to chat. Type your messages:\n");
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
