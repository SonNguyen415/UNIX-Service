#!/bin/bash

SERVER=./server
SRV_LOG="server.log"

CLIENT=./test_client
CLIENT_NAME1="Client1" 
CLIENT_NAME2="Client2"
CLIENT_NAME3="Client3"

MESSAGE_STR1="Test message 1"
MESSAGE_STR2="Test message 2"
MESSAGE_STR3="Test message 3"

# Clear logs
rm -f "$SRV_LOG"

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

# Clients send messages
sleep 1
echo "$MESSAGE_STR1" >&4
echo "$MESSAGE_STR2" >&5
echo "$MESSAGE_STR3" >&6

# Wait a moment to ensure server processes the messages
sleep 1
 
# Kill everyone
kill "$CLIENT3_PID"
kill "$CLIENT2_PID"
kill "$CLIENT1_PID"
kill "$SERVER_PID"

# Wait for the server to terminate 
wait "$SERVER_PID" 2>/dev/null

# Check if all messages are in the client logs
CONDITION1=$(grep "$MESSAGE_STR1" $CLIENT_NAME2.txt && grep "$MESSAGE_STR1" $CLIENT_NAME3.txt)
CONDITION2=$(grep "$MESSAGE_STR2" $CLIENT_NAME1.txt && grep "$MESSAGE_STR2" $CLIENT_NAME3.txt)
CONDITION3=$(grep "$MESSAGE_STR3" $CLIENT_NAME1.txt && grep "$MESSAGE_STR3" $CLIENT_NAME2.txt)


# Check if all conditions are satisfied
if [[ $CONDITION1 != "" && $CONDITION2 != "" && $CONDITION3 != "" ]]; then
    echo "✅ Test 3: PASSED - All messages are present in all client logs"
    exit 0
else
    echo "❌ Test 3: FAILED - One or more messages are missing in the client logs"
    echo "Condition 1 ($MESSAGE_STR1 in Client2 and Client3): $CONDITION1"
    echo "Condition 2 ($MESSAGE_STR2 in Client1 and Client3): $CONDITION2"
    echo "Condition 3 ($MESSAGE_STR3 in Client1 and Client2): $CONDITION3"
    exit 1
fi