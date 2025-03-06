#!/bin/bash 

SERVER=./server
CLIENT=./test_client
SRV_OUTPUT="server.log"
MESSAGE_STR="Test message"
MAX_CLIENTS=5  # Adjust this based on your server's MAX_CLIENTS setting
EXTRA_CLIENT_NAME="ExtraClient"

# Clear logs
rm -f $SRV_OUTPUT

# Start the server in the background
$SERVER &
SERVER_PID=$! 
echo "Server PID: $SERVER_PID"

# Give the server some time to start
sleep 2

# Start MAX_CLIENTS clients
for ((i=1; i<=MAX_CLIENTS; i++)); do
    CLIENT_NAME="Client$i"
    echo "$MESSAGE_STR" | timeout 5s $CLIENT $CLIENT_NAME > /dev/null &
    CLIENT_PIDS[$i]=$!
done

# Give the server some time to process the clients
sleep 2

# Attempt to connect one extra client beyond MAX_CLIENTS
echo "$MESSAGE_STR" | timeout 5s $CLIENT $EXTRA_CLIENT_NAME > /dev/null &
EXTRA_CLIENT_PID=$!

# Wait a moment to ensure the extra client attempts to connect
sleep 2

# Kill the server
kill $SERVER_PID

# Wait for the server to terminate
wait $SERVER_PID 2>/dev/null

# Check if all expected clients (MAX_CLIENTS) show up in the log
PASSED=true
for ((i=1; i<=MAX_CLIENTS; i++)); do
    CLIENT_NAME="Client$i"
    if ! grep -q "$CLIENT_NAME" "$SRV_OUTPUT"; then
        echo "❌ Test: Client $CLIENT_NAME did not appear in the log!"
        PASSED=false
    fi
done

# Check if the extra client was rejected (it should not be in the log)
if grep -q "$EXTRA_CLIENT_NAME" "$SRV_OUTPUT"; then
    echo "❌ Test: Extra client $EXTRA_CLIENT_NAME was incorrectly accepted!"
    PASSED=false
else
    echo "✅ Test: Extra client $EXTRA_CLIENT_NAME was correctly rejected!"
fi

# Report final result
if [ "$PASSED" = true ]; then
    echo "✅ Test 5: PASSED"
    exit 0
else
    echo "❌ Test 5: FAILED"
    exit 1
fi
