#!/bin/bash

SERVER=./server
CLIENT=./test_client
CLIENT_NAME1="Client1" 
CLIENT_NAME2="Client2"
SRV_OUTPUT="server.log"
MESSAGE_STR1="Test message 1"
MESSAGE_STR2="Test message 2"

# Clear logs
rm -f "$SRV_OUTPUT"

# Create named pipes for input
rm -f pipe1 pipe2
mkfifo pipe1 pipe2

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

# Second client sends message before first client
sleep 1
echo "$MESSAGE_STR2" >&5
echo "$MESSAGE_STR1" >&4

# Wait a moment to ensure server processes the messages
sleep 1
 
# Kill everyone
kill "$CLIENT2_PID"
kill "$CLIENT1_PID"
kill "$SERVER_PID"

# Wait for the server to terminate 
wait "$SERVER_PID" 2>/dev/null

# Define conditions for the test - Used Deepseek for this
# Condition 1: Check for two strictly consecutive "New client connected!" lines
CONDITION1=$(awk '
    /New client connected!/ { 
        if (prev == "New client connected!") { 
            found = 1 
        } 
        prev = $0 
    } 
    { 
        if ($0 != "New client connected!") { 
            prev = "" 
        } 
    } 
    END { 
        if (found) print "true"; else print "false" 
    }
' "$SRV_OUTPUT")

# Check if "Broadcasting message from Client2" comes before "Broadcasting message from Client1"
CONDITION2=$(awk '
    /Broadcasting message from Client2/ { found2 = NR }  # Record line number for Client2
    /Broadcasting message from Client1/ { found1 = NR }  # Record line number for Client1
    END {
        if (found2 && found1 && found2 < found1) print "true";  # Client2 before Client1
        else print "false"
    }
' "$SRV_OUTPUT")

# Check if both conditions are satisfied
if [[ "$CONDITION1" == "true" && "$CONDITION2" == "true" ]]; then
    echo "✅ Test: PASSED - Both conditions satisfied"
    exit 0
else
    echo "❌ Test: FAILED - One or both conditions not satisfied"
    echo "Condition 1 (Two consecutive 'New client connected!' lines): $CONDITION1"
    echo "Condition 2 ('Broadcasting message from Client2' before 'Broadcasting message from Client1'): $CONDITION2"
    exit 1
fi