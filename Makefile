CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -g
INCLUDES = -Isrc

SRC = src/main.c src/lexer.c src/parser.c
OBJ = $(SRC:.c=.o)

BIN = krystal

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
