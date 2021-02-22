INCLUDE  := ./include
SRC      := ./src
BIN      := ./bin

CC       := gcc
CC_FLAGS := -std=c17 -Wall -Wextra -Wpedantic -g -I$(INCLUDE)

.PHONY: clean

$(BIN)/main: $(SRC)/main.c $(BIN)
	$(CC) $(CC_FLAGS) -pthread $< -o $(BIN)/main -lpigpiod_if2 -lrt

$(BIN):
	mkdir -p $(BIN)

clean:
	rm $(BIN)/main
