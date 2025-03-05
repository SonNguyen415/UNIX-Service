#!/bin/bash 

SERVER=./server
CLIENT=./test_client
CLIENT_NAME="Client1"
SRV_OUTPUT="server.log"
MESSAGE_STR="Test message"

# Clear logs
rm -f ../$SRV_OUTPUT

# Create server in the background
$SERVER &
SERVER_PID=$! 
echo "Server PID: $SERVER_PID" 
 
# Give the server some time to start up     
sleep 2
 
# Run the client with a 5-second timeout
echo "$MESSAGE_STR" | timeout 5s $CLIENT $CLIENT_NAME > /dev/null

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