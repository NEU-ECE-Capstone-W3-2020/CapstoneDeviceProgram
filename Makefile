INCLUDE  := ./include
SRC      := ./src
BIN      := ./bin

CC       := gcc
CC_FLAGS := -std=c17 -Wall -Wextra -Wpedantic -g -I$(INCLUDE)

.PHONY: clean

$(BIN)/main: $(SRC)/main.c $(BIN)
	$(CC) $(CC_FLAGS) -D_POSIX_C_SOURCE $< -o $(BIN)/main

$(BIN):
	mkdir -p $(BIN)

clean:
	rm $(BIN)/main
