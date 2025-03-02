#include "domain_sockets.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void *receive_messages(void *socket_ptr) {
    int socket_desc = *(int *)socket_ptr;
    struct chat_message msg;

    while (1) {
        if (read(socket_desc, &msg, sizeof(msg)) > 0) {
            printf("%s: %s\n", msg.username, msg.content);
        } else {
            break;
        }
    }
    return NULL;
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

    // Main thread handles sending messages
    struct chat_message msg;
    strncpy(msg.username, argv[1], MAX_USERNAME - 1);

    printf("Connected to chat. Type your messages:\n");
    while (1) {
        if (fgets(msg.content, MAX_MSG_SIZE, stdin) == NULL) break;
        msg.content[strcspn(msg.content, "\n")] = 0;  // Remove newline

        if (strcmp(msg.content, "quit") == 0) break;

        if (write(socket_desc, &msg, sizeof(msg)) == -1) {
            panic("Failed to send message");
        }
    }

    close(socket_desc);
    return 0;
}
