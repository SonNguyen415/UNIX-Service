#!/bin/bash 

CLIENT_NAME="Gabe"
SRV_OUTPUT="server.log"
MESSAGE_STR="Test message"
touch $SRV_OUTPUT

./server &
SERVER_PID=$! 
# Echo the PID in case we need to kill it manually 
echo "Server PID: $SERVER_PID" 
 
# Give the server some time to start up     
sleep 2
 
# Run the client with a 5-second timeout, properly piping the message
echo "$MESSAGE_STR" | timeout 5s ./client $CLIENT_NAME > /dev/null

# Wait a moment to ensure server processes the message
sleep 1
 
# Kill the server
kill $SERVER_PID

# Wait for the server to terminate 
wait $SERVER_PID 2>/dev/null

# Check if the client name shows up in the server log
if grep -q "$CLIENT_NAME" "$SRV_OUTPUT"; then
    echo "✅ Test 1: PASSED"
    exit 0
else
    echo "❌ Test 1: FAILED"
    exit 1
fi