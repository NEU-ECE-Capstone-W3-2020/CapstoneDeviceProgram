INCLUDE  := ./include
SRC      := ./src
BIN      := ./bin

CC       := gcc
CC_FLAGS := -std=c11 -Wall -Wextra -Wpedantic -g -I$(INCLUDE)

.PHONY: clean all

all: $(BIN)/main

$(BIN)/main: $(SRC)/main.c $(BIN)
	$(CC) $(CC_FLAGS) $< -o $(BIN)/main

$(BIN):
	mkdir -p $(BIN)

clean:
	rm $(BIN)/main
