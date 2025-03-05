#!/bin/bash 

SERVER=./server
CLIENT=./test_client
SRV_OUTPUT="server.log"
MESSAGE_STR="Test DM message"
MAX_DM_CONNECTIONS=3  # Adjust this based on your server's MAX_DM_CONNECTIONS setting
EXTRA_DM_CLIENT_NAME="ExtraDMClient"

# Clear logs
rm -f $SRV_OUTPUT

# Start the server in the background
$SERVER &
SERVER_PID=$! 
echo "Server PID: $SERVER_PID"

# Give the server some time to start
sleep 2

# Establish MAX_DM_CONNECTIONS direct message connections
for ((i=1; i<=MAX_DM_CONNECTIONS; i++)); do
    CLIENT_NAME="DMClient$i"
    echo "$MESSAGE_STR to DMClient$i" | timeout 5s $CLIENT $CLIENT_NAME --dm > /dev/null &
    CLIENT_PIDS[$i]=$!
done

# Give the server time to process the DM connections
sleep 2

# Attempt to establish one extra direct message connection beyond MAX_DM_CONNECTIONS
echo "$MESSAGE_STR to ExtraDMClient" | timeout 5s $CLIENT $EXTRA_DM_CLIENT_NAME --dm > /dev/null &
EXTRA_DM_CLIENT_PID=$!

# Allow time for the extra DM connection to be processed
sleep 2

# Kill the server
kill $SERVER_PID

# Wait for the server to terminate
wait $SERVER_PID 2>/dev/null

# Check if all expected DM clients appear in the log
PASSED=true
for ((i=1; i<=MAX_DM_CONNECTIONS; i++)); do
    CLIENT_NAME="DMClient$i"
    if ! grep -q "$CLIENT_NAME" "$SRV_OUTPUT"; then
        echo "❌ Test: DM connection from $CLIENT_NAME did not appear in the log!"
        PASSED=false
    fi
done

# Check if the extra DM connection was rejected (it should NOT be in the log)
if grep -q "$EXTRA_DM_CLIENT_NAME" "$SRV_OUTPUT"; then
    echo "❌ Test: Extra DM connection from $EXTRA_DM_CLIENT_NAME was incorrectly accepted!"
    PASSED=false
else
    echo "✅ Test: Extra DM connection from $EXTRA_DM_CLIENT_NAME was correctly rejected!"
fi

# Report final result
if [ "$PASSED" = true ]; then
    echo "✅ Test 3: PASSED"
    exit 0
else
    echo "❌ Test 3: FAILED"
    exit 1
fi
