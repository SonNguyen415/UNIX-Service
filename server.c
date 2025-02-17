#include <domain_sockets.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>


void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    char *ds = "domain_socket";
    int socket_desc;

    socket_desc = domain_socket_server_create(ds);
    if (socket_desc < 0) {
        /* Remove the previous domain socket file if it exists */
        unlink(ds);
        socket_desc = domain_socket_server_create(ds);
        if (socket_desc < 0) panic("server domain socket creation");
    }

    printf("Server is running and waiting for clients...\n");

    /* Handle multiple clients sequentially */
    /* TODO: Liza change this to handle clients concurrently*/
    while (1) { 
        int new_client;
        char b;

        new_client = accept(socket_desc, NULL, NULL);
        if (new_client == -1) panic("server accept");

        printf("Client connected!\n");

        /* Read from, then write to the client */
        if (read(new_client, &b, 1) == -1) panic("server read");
        if (write(new_client, "*", 1) == -1) panic("server write");
        printf("Server read from client: %c\n", b);
        close(new_client);
    }

    close(socket_desc);
    printf("Server shutting down...\n");

    return 0;
}
