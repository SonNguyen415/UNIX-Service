#include <domain_sockets.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void client(char *filename) {
    int socket_desc;
    char b;

    socket_desc = domain_socket_client_create(filename);
    if (socket_desc < 0) {
        perror("domain socket client create");
        exit(EXIT_FAILURE);
    }

    if (write(socket_desc, ".", 1) == -1) panic("client write");
    if (read(socket_desc, &b, 1) == -1)   panic("client read");
    printf("Client read from server: %c\n", b);

    close(socket_desc);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <socket_filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    client(argv[1]);
    return EXIT_SUCCESS;
}
