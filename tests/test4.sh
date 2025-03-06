#!/bin/bash

SERVER=./server
CLIENT_NAME3="Client3"
SRV_OUTPUT="server.log"

CLIENT=./test_client
CLIENT_NAME1="Client1" 
CLIENT_NAME2="Client2"
MESSAGE_STR1="@$CLIENT_NAME2 Test message 1"
MESSAGE_STR2="@$CLIENT_NAME1 Test message 2"

# Clear logs
rm -f "$SRV_OUTPUT"

# Create named pipes for input
rm -f pipe1 pipe2 pipe3
mkfifo pipe1 pipe2 pipe3

# Start server in the background
$SERVER &
SERVER_PID=$! 
echo "Server PID: $SERVER_PID"    
sleep 1

# Start first client using named pipe for input
$CLIENT "$CLIENT_NAME1" < pipe1 &
CLIENT1_PID=$!
exec 4>pipe1  

# Start second client using named pipe for input
$CLIENT "$CLIENT_NAME2" < pipe2 &
CLIENT2_PID=$!
exec 5>pipe2  

# Start third client using named pipe for input
$CLIENT "$CLIENT_NAME3" < pipe3 &
CLIENT3_PID=$!
exec 6>pipe3

sleep 1

# HACK: Client sent messages to register (h)
echo "Register" >&4
echo "Register" >&5
echo "Register" >&6

# DM messages
echo "$MESSAGE_STR1" >&4
echo "$MESSAGE_STR2" >&5

# Wait a moment to ensure server processes the messages
sleep 1
 
# Kill everyone
kill "$CLIENT3_PID"
kill "$CLIENT2_PID"
kill "$CLIENT1_PID"
kill "$SERVER_PID"

# Wait for the server to terminate 
wait "$SERVER_PID" 2>/dev/null

# Count lines in each log file
LINES_CLIENT1=$(wc -l < Client1.txt)
LINES_CLIENT2=$(wc -l < Client2.txt)

# Check if messages are in client1 and client2 but not client3
CONDITION1=$(grep "$MESSAGE_STR1" Client2.txt && grep "$MESSAGE_STR2" Client1.txt)
CONDITION2=$(grep "$MESSAGE_STR1" Client3.txt && grep "$MESSAGE_STR2" Client3.txt)

# Check if all conditions are satisfied
if [[ "$LINES_CLIENT1" -eq 4 && "$LINES_CLIENT2" -eq 4 && "$CONDITION2" == "" ]]; then
    echo "✅ Test 4: PASSED - All messages are present in all client logs"
    exit 0
else
    echo "❌ Test 4: FAILED - "
    echo "Condition 1 (Messages in Client1 and Client2): $CONDITION1"
    echo "Condition 2 (Messages not in Client3): $CONDITION2"
    exit 1
fi