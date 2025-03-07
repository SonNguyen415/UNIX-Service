#pragma once

/*
 * Graciously taken from
 * https://github.com/troydhanson/network/tree/master/unixdomain
 */

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#define _GNU_SOURCE // Needed for SCM_CREDENTIALS

// Define ucred structure if not defined
struct ucred {
  pid_t pid; /* Process ID of the sending process */
  uid_t uid; /* User ID of the sending process */
  gid_t gid; /* Group ID of the sending process */
};

// Max message size
#define MAX_MSG_SIZE 1024
// Max username length
#define MAX_USERNAME 32

// Chat message struct
struct chat_message {
  char username[MAX_USERNAME];
  char content[MAX_MSG_SIZE];
  char target[MAX_USERNAME]; // Target username for DMs, empty for broadcast
  int is_dm;                 // Flag to indicate if this is a DM
};

// Structure to hold client credentials
struct client_credentials {
  uid_t uid;          // User ID
  gid_t gid;          // Group ID
  pid_t pid;          // Process ID
  char username[256]; // System username
};

// Function to get client credentials
static inline int get_client_credentials(int socket_fd,
                                         struct client_credentials *cred) {
  struct ucred peer_cred;
  socklen_t len = sizeof(struct ucred);

  // Use SO_PEERCRED to get credentials
  if (getsockopt(socket_fd, SOL_SOCKET, SO_PEERCRED, &peer_cred, &len) == -1) {
    return -1;
  }

  cred->uid = peer_cred.uid;
  cred->gid = peer_cred.gid;
  cred->pid = peer_cred.pid;

  // Get system username
  struct passwd *pw = getpwuid(peer_cred.uid);
  if (pw) {
    strncpy(cred->username, pw->pw_name, sizeof(cred->username) - 1);
  } else {
    snprintf(cred->username, sizeof(cred->username), "%d", peer_cred.uid);
  }

  return 0;
}

// Create a client socket
static inline int domain_socket_client_create(const char *file_name) {
  struct sockaddr_un addr;
  int fd;

  /* Create the socket descriptor.  */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    return -1;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, file_name, sizeof(addr.sun_path) - 1);

  /* Attempt to connect the socket descriptor with a socket file named
   * `file_name`. */
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    return -1;

  return fd;
}

// Create a server socket
static inline int domain_socket_server_create(const char *file_name) {
  struct sockaddr_un addr;
  int fd;

  /* Create the socket descriptor.  */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    return -1;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, file_name, sizeof(addr.sun_path) - 1);

  /* Associate the socket descriptor with a socket file named `file_name`. */
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    return -1;
  /* How many clients can the system queue up before they are `accept`ed? */
  if (listen(fd, 5) == -1)
    return -1;

  return fd;
}

// Send a file descriptor over a socket
// https://gist.github.com/domfarolino/4293951bd95082125f2b9931cab1de40
static inline int send_fd(int socket, int fd_to_send) {
  struct msghdr msg = {0};
  struct cmsghdr *cmsg;
  char buf[CMSG_SPACE(sizeof(int))] = {0};
  struct iovec io = {.iov_base = "", .iov_len = 1};

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = buf;
  msg.msg_controllen = sizeof(buf);

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));

  *((int *)CMSG_DATA(cmsg)) = fd_to_send;

  return sendmsg(socket, &msg, 0);
}

// Receive a file descriptor over a socket
static inline int recv_fd(int socket) {
  struct msghdr msg = {0};
  struct cmsghdr *cmsg;
  char buf[CMSG_SPACE(sizeof(int))] = {0};
  struct iovec io = {.iov_base = "", .iov_len = 1};
  int fd;

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = buf;
  msg.msg_controllen = sizeof(buf);

  if (recvmsg(socket, &msg, 0) < 0)
    return -1;

  cmsg = CMSG_FIRSTHDR(&msg);
  fd = *((int *)CMSG_DATA(cmsg));
  return fd;
}

// Send an empty message in order to register user
static inline int register_user(int socket_desc, struct chat_message *msg) {
  msg->content[0] = '\0';
  msg->is_dm = 0;
  return write(socket_desc, msg, sizeof(struct chat_message));
}