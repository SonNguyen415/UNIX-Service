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
#define BUF_SIZE 100
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
		return -1;
	
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
	memcpy(CMSG_DATA(cmsgp), &fd, fdAllocSize);

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
	struct msghdr msgh = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(int))] = {0};
	struct iovec io = { .iov_base = "", .iov_len = 1 };
	int fd;

	msgh.msg_iov = &io;
	msgh.msg_iovlen = 1;
	msgh.msg_control = buf;
	msgh.msg_controllen = sizeof(buf);

	size_t controlMsgSize = CMSG_SPACE(sizeof(int[MAX_FDS])) +
	CMSG_SPACE(sizeof(struct ucred));
	char *controlMsg = malloc(controlMsgSize);
	if (controlMsg == NULL)
		return -1;



	if (recvmsg(socket, &msgh, 0) < 0) return -1;

	for (struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh); cmsgp != NULL; cmsgp = CMSG_NXTHDR(&msgh, cmsgp)) {

		printf("=================================\n");
		printf("cmsg_len: %ld\n", (long) cmsgp->cmsg_len);

		/* Check that 'cmsg_level' is as expected */

			if (cmsgp->cmsg_level != SOL_SOCKET)
				return -1;

			switch (cmsgp->cmsg_type) {

			case SCM_RIGHTS:        /* Header containing file descriptors */

			printf("SCM_RIGHTS: ");

			/* The number of file descriptors is the size of the control
				message block minus the size that would be allocated for
				a zero-length data block (i.e., the size of the 'cmsghdr'
				structure plus padding), divided by the size of a file
				descriptor */

			int fdCnt = (cmsgp->cmsg_len - CMSG_LEN(0)) / sizeof(int);
			printf("received %d file descriptors\n", fdCnt);

			/* Allocate an array to hold the received file descriptors,
				and copy file descriptors from cmsg into array */

			int *fdList;
			size_t fdAllocSize = sizeof(int) * fdCnt;
			fdList = malloc(fdAllocSize);
			if (fdList == NULL){
				return -1;
			}
			memcpy(fdList, CMSG_DATA(cmsgp), fdAllocSize);

			/* For each of the received file descriptors, display the file
				descriptor number and read and display the file content */

				for (struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
				cmsgp != NULL;
				cmsgp = CMSG_NXTHDR(&msgh, cmsgp)) {

		printf("=================================\n");
		printf("cmsg_len: %ld\n", (long) cmsgp->cmsg_len);

		/* Check that 'cmsg_level' is as expected */

		if (cmsgp->cmsg_level != SOL_SOCKET)
			return -1; //fatal("cmsg_level != SOL_SOCKET");

		switch (cmsgp->cmsg_type) {

		case SCM_RIGHTS:        /* Header containing file descriptors */

			printf("SCM_RIGHTS: ");

			/* The number of file descriptors is the size of the control
				message block minus the size that would be allocated for
				a zero-length data block (i.e., the size of the 'cmsghdr'
				structure plus padding), divided by the size of a file
				descriptor */

			int fdCnt = (cmsgp->cmsg_len - CMSG_LEN(0)) / sizeof(int);
			printf("received %d file descriptors\n", fdCnt);

			/* Allocate an array to hold the received file descriptors,
				and copy file descriptors from cmsg into array */

			int *fdList;
			size_t fdAllocSize = sizeof(int) * fdCnt;
			fdList = malloc(fdAllocSize);
			if (fdList == NULL)
				return -1; //errExit("calloc");

			memcpy(fdList, CMSG_DATA(cmsgp), fdAllocSize);

			/* For each of the received file descriptors, display the file
				descriptor number and read and display the file content */

			for (int j = 0; j < fdCnt; j++) {
				printf("--- [%d] Received FD %d\n", j, fdList[j]);

				for (;;) {
					char buf[BUF_SIZE];
					ssize_t numRead;

					numRead = read(fdList[j], buf, BUF_SIZE);
					if (numRead == -1)
						return -1; //errExit("read");

					if (numRead == 0)
						break;

					write(STDOUT_FILENO, buf, numRead);
				}

				if (close(fdList[j]) == -1)
					return -1; //errExit("close");
			}
			break;

		case SCM_CREDENTIALS:   /* Header containing credentials */

			/* Check validity of the 'cmsghdr' */

			if (cmsgp->cmsg_len != CMSG_LEN(sizeof(struct ucred)))
				return -1; //fatal("cmsg data has incorrect size");

			/* The data in this control message block is a 'struct ucred' */

			struct ucred creds;
			memcpy(&creds, CMSG_DATA(cmsgp), sizeof(struct ucred));
			printf("SCM_CREDENTIALS: pid=%ld, uid=%ld, gid=%ld\n",
						(long) creds.pid, (long) creds.uid, (long) creds.gid);
			break;

		default:
			return -1; //fatal("Bad cmsg_type (%d)", cmsgp->cmsg_type);
	}
}









	cmsg = CMSG_FIRSTHDR(&msgh);
	fd = *((int *) CMSG_DATA(cmsg));
	return fd;
}

// Send an empty message in order to register user
static inline int register_user(int socket_desc, struct chat_message *msg) {
	msg->content[0] = '\0';
	msg->is_dm = 0;
	return write(socket_desc, msg, sizeof(struct chat_message));
}