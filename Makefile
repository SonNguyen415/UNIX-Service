CC = gcc
CFLAGS = -Wall -Wextra -I.

SERVER_SRC = server.c
CLIENT_SRC = client.c

SERVER_BIN = server
CLIENT_BIN = client
LOG = server.log

TEST_DIR = tests
TESTS = $(wildcard $(TEST_DIR)/*.sh) 

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC) -pthread


test: $(SERVER_BIN) $(CLIENT_BIN)
	make run_tests

run_tests: $(TESTS)
	@for script in $^; do \
		echo "Running $$script..."; \
		chmod +x $$script; \
		./$$script; \
	done


clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(LOG)
	find $(TEST_DIR) -type f ! -name "*.sh" -delete
