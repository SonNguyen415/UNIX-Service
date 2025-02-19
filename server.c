#include <domain_sockets.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>


#define MAX_EVENTS 10

void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void panic(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    char *ds = "domain_socket";
    int socket_desc, epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];

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
    set_nonblocking(socket_desc);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) panic("epoll_create1");

    event.data.fd = socket_desc;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_desc, &event) == -1) panic("epoll_ctl");

    printf("Server is running and waiting for clients...\n");

    // notes from liza: used chatGPT to aid with event loop creation
    while (1) {
        int n, i;
        n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) panic("epoll_wait");

        for (i = 0; i < n; i++) {
            if (events[i].data.fd == socket_desc) {
                int new_client;
                new_client = accept(socket_desc, NULL, NULL);
                if (new_client == -1) continue;
                set_nonblocking(new_client);

                event.data.fd = new_client;
                event.events = EPOLLIN;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client, &event);

                printf("Client connected!\n");
            } else {
                int client_fd = events[i].data.fd;
                char b;
                if (read(client_fd, &b, 1) > 0) {
                    write(client_fd, "*", 1);
                    printf("Server read from client: %c\n", b);
                }
                close(client_fd);
            }
        }
    }

    close(socket_desc);
    close(epoll_fd);
    close(socket_desc);
    printf("Server shutting down...\n");

    return 0;
}
