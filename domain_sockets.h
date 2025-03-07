#pragma once
#define _GNU_SOURCE
/*
 * Graciously taken from
 * https://github.com/troydhanson/network/tree/master/unixdomain
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include "scm_multi.h"

// Max message size
#define MAX_MSG_SIZE 1024
// Max username length
#define MAX_USERNAME 32

// Chat message struct
struct chat_message {
	char username[MAX_USERNAME];
	char content[MAX_MSG_SIZE];
	char target[MAX_USERNAME];  // Target username for DMs, empty for broadcast
	int is_dm;                 // Flag to indicate if this is a DM
};
// Create a client socket
static inline int
domain_socket_client_create(const char *file_name)
{
	// Adapted from slides 
	

	pid_t pid = getpid();
	uid_t uid = getuid();
	gid_t gid = getgid();

	size_t fdAllocSize = sizeof(int);
	size_t controlMsgSize = CMSG_SPACE(fdAllocSize) + CMSG_SPACE(sizeof(struct ucred));
	char *controlMsg = malloc(controlMsgSize);
	if (controlMsg == NULL)
		return NULL;
	
	memset(controlMsg,0, controlMsgSize);

	struct msghdr msgh;
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

	msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;

	struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
	cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;

	cmsgp->cmsg_len = CMSG_LEN(fdAllocSize);
    //printf("cmsg_len 1: %ld\n", (long) cmsgp->cmsg_len);

	//int *fdList = malloc(fdAllocSize);


	//open the fd 
	struct sockaddr_un addr;
	int fd;

	/* Create the socket descriptor.  */
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) return -1;

	//copy it into the message to send.
	memcpy(CMSG_DATA(cmsgp), fd, fdAllocSize);

	//copy credentials
	cmsgp = CMSG_NXTHDR(&msgh, cmsgp);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_CREDENTIALS;

	//add space for ucred
	cmsgp->cmsg_len = CMSG_LEN(sizeof(struct ucred));

	struct ucred creds;
	creds.pid = pid;
	creds.uid = uid;
    creds.gid = gid;

	//copy creds
	memcpy(CMSG_DATA(cmsgp), &creds, sizeof(struct ucred));



	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, file_name, sizeof(addr.sun_path) - 1);

	/* Attempt to connect the socket descriptor with a socket file named `file_name`. */
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) return -1;

	ssize_t ns = sendmsg(fd, &msgh, 0);
    if (ns == -1)
		return -1;
	return fd;
}

// Create a server socket
static inline int
domain_socket_server_create(const char *file_name)
{


	struct sockaddr_un addr;
	int fd;

	/* Create the socket descriptor.  */
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, file_name, sizeof(addr.sun_path) - 1);

	/* Associate the socket descriptor with a socket file named `file_name`. */
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) return -1;
	/* How many clients can the system queue up before they are `accept`ed? */
	if (listen(fd, 5) == -1) return -1;

	return fd;
}

// Send a file descriptor over a socket https://gist.github.com/domfarolino/4293951bd95082125f2b9931cab1de40
static inline int send_fd(int socket, int fd_to_send) {
	struct msghdr msg = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(int))] = {0};
	struct iovec io = { .iov_base = "", .iov_len = 1 };

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));

	*((int *) CMSG_DATA(cmsg)) = fd_to_send;

	return sendmsg(socket, &msg, 0);
}

// Receive a file descriptor over a socket
static inline int recv_fd(int socket) {
	struct msghdr msg = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(int))] = {0};
	struct iovec io = { .iov_base = "", .iov_len = 1 };
	int fd;

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	size_t controlMsgSize = CMSG_SPACE(sizeof(int[MAX_FDS])) +
	CMSG_SPACE(sizeof(struct ucred));
	char *controlMsg = malloc(controlMsgSize);
	if (controlMsg == NULL)
		return -1;



	if (recvmsg(socket, &msg, 0) < 0) return -1;

	cmsg = CMSG_FIRSTHDR(&msg);
	fd = *((int *) CMSG_DATA(cmsg));
	return fd;
}

// Send an empty message in order to register user
static inline int register_user(int socket_desc, struct chat_message *msg) {
	msg->content[0] = '\0';
	msg->is_dm = 0;
	return write(socket_desc, msg, sizeof(struct chat_message));
}