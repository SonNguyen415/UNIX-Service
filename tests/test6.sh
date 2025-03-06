#!/bin/bash 

SERVER=./server
CLIENT=./test_client
SRV_LOG="server.log"
MESSAGE_STR="Test message"
CLIENT_NAME="DMClient"
FAKE_CLIENT_NAME="FakeClient"

# Clear logs
rm -f $SRV_LOG

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

CONDITION1=false
if grep -q "User '$FAKE_CLIENT_NAME' is not online" "$CLIENT_NAME.txt"; then
    CONDITION1=true
fi

CONDITION2=false
if grep -q "Failed DM" "$SRV_LOG"; then
    CONDITION2=true
fi

# Report final result
if [[ $CONDITION1 = true && $CONDITION2 = true ]]; then
    echo "✅ Test 6: PASSED - Client failed to send DM to offline user"
    exit 0
else
    echo "❌ Test 6: FAILED - Client sent DM to offline user"
    exit 1
fi
