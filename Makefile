CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC = chunk.c debug.c memory.c value.c vm.c scanner.c compiler.c main.c
OBJ = $(SRC:.c=.o)
TARGET = clox

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ)
