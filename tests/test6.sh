#!/bin/bash 

SERVER=./server
CLIENT=./test_client
SRV_OUTPUT="server.log"
MESSAGE_STR="Test message"
CLIENT_NAME="DMClient"
FAKE_CLIENT_NAME="FakeClient"

# Clear logs
rm -f $SRV_OUTPUT

# Start the server in the background
$SERVER &
SERVER_PID=$! 
echo "Server PID: $SERVER_PID"

# Give the server some time to start
sleep 2

# Start a client for DM
(echo "@$FAKE_CLIENT_NAME $MESSAGE_STR"; sleep 5) | $CLIENT "$CLIENT_NAME" &
CLIENT_PID=$!

# Give the server some time to process the clients
sleep 2

# Kill everyone
kill $CLIENT_PID
kill $SERVER_PID

# Wait for the server to terminate
wait $SERVER_PID 2>/dev/null

# Check if the message is in the log
PASSED=false
if grep "User '$FAKE_CLIENT_NAME' is not online" "${CLIENT_NAME}.txt"; then
    PASSED=true
fi

CONDITION1=$(grep "User '$FAKE_CLIENT_NAME' is not online" $CLIENT_NAME.txt)
CONDITION2=$(grep "Failed DM" $SRV_OUTPUT)

# Report final result
if [ "$PASSED" = true ]; then
    echo "✅ Test 6: PASSED"
    exit 0
else
    echo "❌ Test 6: FAILED"
    exit 1
fi
