CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -g
INCLUDES = -Isrc
LIBS = src/lexer.c src/parser.c
SRC = src/main.c $(LIBS)
DIR = build
OBJ = $(patsubst src/%.c, $(DIR)/%.o, $(SRC))
BIN = $(DIR)/krystal

all: $(BIN)

$(DIR):
	@mkdir -p $(DIR)

$(BIN): $(OBJ) | $(DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(DIR)/%.o: src/%.c | $(DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
