#!/bin/bash 

SRV_OUTPUT="output.txt"
MESSAGE_STR="Test message"
touch $SRV_OUTPUT

./server &
SERVER_PID=$! 
# Echo the PID in case we need to kill it manually 
echo "Server PID: $SERVER_PID" 

 
# Give the server some time to start up     
sleep 2
 
# Run the client with a 5-second timeout, properly piping the message
echo "$MESSAGE_STR" | timeout 5s ./client son

# Wait a moment to ensure server processes the message
sleep 1
 
# Kill the server
kill $SERVER_PID

# Wait for the server to terminate 
wait $SERVER_PID 2>/dev/null

# Check if the original message is in the server's output
if grep -q "$MESSAGE_STR" "$SRV_OUTPUT"; then
    echo "Message successfully broadcasted!"
    exit 0
else
    echo "Message not found in server output."
    exit 1
fi