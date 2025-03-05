CC = gcc
CFLAGS = -Wall -Wextra -I.

SERVER_SRC = server.c
CLIENT_SRC = client.c
TEST_CLIENT = test_client.c

TEST_BIN = test_client
SERVER_BIN = server
CLIENT_BIN = client
LOG = server.log
SOCKET = chat_socket

TEST_DIR = tests
TESTS = $(sort $(wildcard $(TEST_DIR)/test*.sh))  # Ensures files are sorted

all: $(SERVER_BIN) $(CLIENT_BIN)

test: $(SERVER_BIN) $(TEST_BIN)
	@make --no-print-directory run_tests

run_tests: $(TESTS)
	@for script in $^; do \
		echo "Running $$script..."; \
		chmod +x $$script; \
		./$$script; \
	done

	
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN) $(LOG) $(SOCKET)
	rm -f $(wildcard pipe*) 
	rm -f $(wildcard *.txt)
	find $(TEST_DIR) -type f ! -name "*.sh" -delete

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC) -pthread

$(TEST_BIN): $(TEST_CLIENT)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_CLIENT) -pthread
