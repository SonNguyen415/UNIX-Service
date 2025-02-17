#include <domain_sockets.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>

void
panic(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void
client(char *filename, int slowdown)
{
    int socket_desc;
    char b;

    socket_desc = domain_socket_client_create(filename);
    if (socket_desc < 0) {
        perror("domain socket client create");
        exit(EXIT_FAILURE);
    }

    /* delay after creating connection, but before communicating */
    sleep(slowdown);
    if (write(socket_desc, ".", 1) == -1) panic("client write");
    if (read(socket_desc, &b, 1) == -1)   panic("client read");
    printf("Client read from server: %c\n", b);

    close(socket_desc);
    exit(EXIT_SUCCESS);
}

void
client_slow(char *filename)
{
    client(filename, 3);
}

void
client_fast(char *filename)
{
    client(filename, 1);
}

int
main(void)
{
    char *ds = "domain_socket";
    int socket_desc, i;

    socket_desc = domain_socket_server_create(ds);
    if (socket_desc < 0) {
        /* remove the previous domain socket file if it exists */
        unlink(ds);
        socket_desc = domain_socket_server_create(ds);
        if (socket_desc < 0) panic("server domain socket creation");
    }

    /* TODO: change this order. What changes? */
    if (fork() == 0) client_slow(ds);
    if (fork() == 0) client_fast(ds);

    /* handle two clients, one after the other */
    for (i = 0; i < 2; i++) {
        int new_client;
        char b;

        new_client = accept(socket_desc, NULL, NULL);
        if (new_client == -1) panic("server accept");

        /* read from, then write to the client! */
        if (read(new_client, &b, 1) == -1)   panic("server read");
        if (write(new_client, "*", 1) == -1) panic("server write");
        printf("Server read from client: %c\n", b);
        close(new_client);
    }

    close(socket_desc);
    /* reap all children */
    while (wait(NULL) != -1) ;

    return 0;
}
