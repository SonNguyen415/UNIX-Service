CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS =
TARGET = test
SRC = main.c  
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)