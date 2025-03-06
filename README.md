# UNIX-Service


## Core Requirements

Your service must do all of the following:

1. Use UNIX domain sockets for communication with a known path for the *name* of the socket.
2. Use `accept` to create a connection-per-client as in [here](https://gwu-cs-advos.github.io/sysprog/#communication-with-multiple-clients).
3. Use event notification (epoll, poll, select, etc...) to manage concurrency as in [here](https://gwu-cs-advos.github.io/sysprog/#event-loops-and-poll).
4. Use domain socket facilities to get a trustworthy identity of the client (i.e. user id).
5. Pass file descriptors between the service and the client.

[The Linux Programming Interface](https://man7.org/tlpi/code/index.html) is a great resource for code for domain sockets and related technologies.
For example, see the [client](https://man7.org/tlpi/code/online/dist/sockets/ud_ucase_cl.c.html) and [server](https://man7.org/tlpi/code/online/dist/sockets/ud_ucase_sv.c.html) code for 1. getting authenticated client id, and 2. [passing](https://man7.org/tlpi/code/online/dist/sockets/scm_multi_send.c.html) [file-descriptors](https://man7.org/tlpi/code/online/dist/sockets/scm_multi_recv.c.html) from the service to the client.


## Tests
- Test 1: Client connect to and send message to server
- Test 2: Concurrency test, 1 client makes no request for a few seconds, the other then connect and make requests.
- Test 3: Broadcast test, 3 clients connect, each client sends a message. Message should be seen by all 3 clients.
- Test 4: DM test. 3 clients connect, client A DM to client B, client C should not see message.
- Test 5: Test what happens if more clients than MAX_CLIENTS attempt to connect.
- Test 6: Test what happens if 
- Test 7: Test what happens if a client dm with an invalid id.

## Known Bugs
1. Because of lack of UI, when you receive a message in the middle of writing your own, the received message interleave in your current message. Low priority because this seems to be client side functionality.
- I forgot Yuan add your shit here


## Task Lists
### Primary Tasks
[x] Use UNIX domain sockets for communication with a known path for the *name* of the socket.
[x] Use `accept` to create a connection-per-client as in [here](https://gwu-cs-advos.github.io/sysprog/#communication-with-multiple-clients).
[x] Use event notification (epoll, poll, select, etc...) to manage concurrency as in [here](https://gwu-cs-advos.github.io/sysprog/#event-loops-and-poll).
[] Use domain socket facilities to get a trustworthy identity of the client (i.e. user id).
[x] Pass file descriptors between the service and the client.
[x] Implement global chatroom between multiple clients.
[x] Implement DM services between multiple clients.

### If we have time
[] Fix bug #1.
[] Use network sockets for communication between different hosts.
[x] Remove our strcpy and change them to strncpy they bother my OCD.
[] Add more security???