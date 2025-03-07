#include "domain_sockets.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define MAX_CLIENTS 10
#define LOG_FILE "server.log"

struct user_info {
  char username[MAX_USERNAME];
  int socket_fd;
};

struct user_info users[MAX_CLIENTS];

int num_clients = 0;

void log_message(const char *format, ...) {
  // Open file in append mode
  FILE *log_file = fopen(LOG_FILE, "a");
  if (!log_file) {
    perror("Error opening log file for writing");
    return;
  }

  // Create a va_list to handle variable arguments
  va_list args;
  va_start(args, format);

  // Use vfprintf to process variable arguments like log_message
  vfprintf(log_file, format, args);
  if (format[strlen(format) - 1] != '\n') {
    fprintf(log_file, "\n");
  }

  // Clean up the va_list and close the file
  va_end(args);
  fclose(log_file);
}

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
      log_message("Removing user %s with fd %d", users[i].username, client_fd);
      users[i] = users[num_clients - 1];
      num_clients--;
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
  // First message handling (username registration)
  for (int i = 0; i < num_clients; i++) {
    if (users[i].socket_fd == sender_fd && users[i].username[0] == '\0') {
      strncpy(users[i].username, msg->username, MAX_USERNAME - 1);
      log_message("User %s registered with fd %d", msg->username, sender_fd);
      return;  // Return after registration, don't process empty message
    }
  }

  // If empty message, then don't bother
  if (msg->content[0] == '\0') {
    return;  // Don't process empty messages at all
  }

  if (msg->is_dm) {
    int target_fd = find_user_socket(msg->target);
    if (target_fd == -1) {
      // User not found - send error message back to sender
      struct chat_message error_msg = {0};
      error_msg.is_dm = 1;
      strncpy(error_msg.username, "SERVER", MAX_USERNAME - 1);
      snprintf(error_msg.content, MAX_MSG_SIZE, "User '%s' is not online.",
               msg->target);
      write(sender_fd, &error_msg, sizeof(error_msg));
      log_message("Failed DM from %s to non-existent user %s", msg->username,
                  msg->target);
      return;
    }

    // Pass file descriptors between clients
    if (send_fd(sender_fd, target_fd) < 0) {
      log_message("Failed to send fd to sender");
      return;
    }
    
    // Send setup message and fd to target
    struct chat_message setup_msg = {0};
    setup_msg.is_dm = 2;  // DM setup flag
    strncpy(setup_msg.username, msg->username, MAX_USERNAME - 1);
    write(target_fd, &setup_msg, sizeof(setup_msg));
    if (send_fd(target_fd, sender_fd) < 0) {
      log_message("Failed to send fd to target");
      return;
    }

    // Send the original message through the server this first time
    write(target_fd, msg, sizeof(struct chat_message));
    log_message("DM setup complete");
  } else {
    // Regular broadcast
    log_message("Broadcasting message from %s: %s", msg->username,
                msg->content);
    for (int i = 0; i < num_clients; i++) {
      if (users[i].socket_fd != sender_fd) {
        write(users[i].socket_fd, msg, sizeof(struct chat_message));
      }
    }
  }
}

int main(void) {

  // Create a log
  FILE *log_file = fopen(LOG_FILE, "w");
  if (!log_file) {
    perror("Failed to open log file");
    return -1;
  }
  fclose(log_file);

  char *ds = "chat_socket";
  int socket_desc, epoll_fd;
  struct epoll_event event, events[MAX_EVENTS];

  socket_desc = domain_socket_server_create(ds);
  if (socket_desc < 0) {
    /* Remove the previous domain socket file if it exists */
    unlink(ds);
    socket_desc = domain_socket_server_create(ds);
    if (socket_desc < 0)
      panic("server domain socket creation");
  }

  log_message("Chat server is running...\n");

  /* Handle multiple clients sequentially */
  /* TODO: Liza change this to handle clients concurrently*/
  set_nonblocking(socket_desc);

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1)
    panic("epoll_create1");

  event.data.fd = socket_desc;
  event.events = EPOLLIN;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_desc, &event) == -1)
    panic("epoll_ctl");

  // notes from liza: used chatGPT to aid with event loop creation
  while (1) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n == -1)
      panic("epoll_wait");

    for (int i = 0; i < n; i++) {
      if (events[i].data.fd == socket_desc) {
        // New connection
        if (num_clients >= MAX_CLIENTS) {
          log_message("Max clients reached, rejecting new connection");
          continue;
        }

        int client_fd = accept(socket_desc, NULL, NULL);
        if (client_fd == -1)
          continue;

        set_nonblocking(client_fd);
        event.data.fd = client_fd;
        event.events = EPOLLIN;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
        add_client(client_fd, "");
        log_message("New client connected\n");
      } else {
        // Existing client sent data
        int client_fd = events[i].data.fd;
        struct chat_message msg;

        int bytes = read(client_fd, &msg, sizeof(msg));
        if (bytes <= 0) {
          // Client disconnected
          remove_client(client_fd);
          close(client_fd);
          log_message("Client disconnected\n");
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
  log_message("Chat server shutting down...\n");

  return 0;
}
