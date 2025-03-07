# UNIX-Service Chat Room 
Our goal for this project was to implement global chatroom between multiple client and to implement DM services between multiple clients. We have also implemented private user direct messaging.

## Using our Chatroom
1. Compile our files using `make`
2. run `./client` and `./server <desired_username>` in seperate terminals
3. run `./client <desired_username>` in multiple seperate terminals with different users
4. To globally message every simply type `<desired_message>` into the terminal and hit send.
      <img width="400" alt="Screenshot 2025-03-06 at 7 14 37 PM" src="https://github.com/user-attachments/assets/c8557ff8-4809-4e1c-9931-667155f71516" />
5. To privately direct message type `@<username> <desired_message>`
  <img width="357" alt="Screenshot 2025-03-06 at 7 12 54 PM" src="https://github.com/user-attachments/assets/5470e574-2702-406e-ba90-b8bc17676516" />

## Tests
To run our tests type and enter `make test` into the terminal and this will run and output the below cases:
- Test 1: Client connect to and send message to server
- Test 2: Concurrency test, 1 client makes no request for a few seconds, the other then connect and make requests.
- Test 3: Broadcast test, 3 clients connect, each client sends a message. Message should be seen by all 3 clients.
- Test 4: DM test. 3 clients connect, client A DM to client B, client C should not see message.
- Test 5: Test what happens if more clients than MAX_CLIENTS attempt to connect.
- Test 6: Test what happens if user tries to send a DM to an offline user.
- Test 7: Test what happens if a client dm with an invalid id.

## Known Bugs
1. Because of lack of UI, when you receive a message in the middle of writing your own, the received message interleave in your current message. Low priority because this seems to be client side functionality.

## Assumptions
- we assume that a max number of users on a server is 10
