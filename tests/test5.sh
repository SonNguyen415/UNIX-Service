#!/bin/bash 

SERVER=./server
CLIENT=./test_client
SRV_LOG="server.log"
MESSAGE_STR="Test message"
MAX_CLIENTS=10  
EXTRA_CLIENT_NAME="ExtraClient"

# Clear logs
rm -f $SRV_LOG

# Start the server in the background
$SERVER &
SERVER_PID=$! 
echo "Server PID: $SERVER_PID"

# Give the server some time to start
sleep 2

# Start MAX_CLIENTS clients
for ((i=1; i<=MAX_CLIENTS; i++)); do
    CLIENT_NAME="Client$i"
    (echo "$MESSAGE_STR"; sleep 5) | $CLIENT "$CLIENT_NAME" > /dev/null &
    CLIENT_PIDS[$i]=$!
done

# Give the server some time to process the clients
sleep 2

# Attempt to connect one extra client beyond MAX_CLIENTS
echo "$MESSAGE_STR" | timeout 1s $CLIENT $EXTRA_CLIENT_NAME > /dev/null &
EXTRA_CLIENT_PID=$!

# Wait a moment to ensure the extra client attempts to connect
sleep 2

# Kill the server
kill $SERVER_PID

# Wait for the server to terminate
wait $SERVER_PID 2>/dev/null

# Check if all expected clients (MAX_CLIENTS) show up in the log
CONDITION1=true
for ((i=1; i<=MAX_CLIENTS; i++)); do
    CLIENT_NAME="Client$i"
    if ! grep -q "$CLIENT_NAME" "$SRV_LOG"; then
        CONDITION1=false
    fi
done

# Check if the extra client was rejected (it should not be in the log)
CONDITION2=true
if grep -q "$EXTRA_CLIENT_NAME" "$SRV_LOG"; then
    CONDITION2=false
fi

# Report final result
if [[ $CONDITION1 = true && $CONDITION2 = true ]]; then
    echo "✅ Test 5: PASSED - Server rejects client more than max"
    exit 0
else
    echo "❌ Test 5: FAILED"
    echo "All clients within limit connected successfully: $CONDITION1"
    echo "Extra client was rejected: $CONDITION2"
    exit 1
fi
