INCLUDE  := ./include
SRC      := ./src
BIN      := ./bin
PIGPIO   := ../pigpio

CC       := gcc
CC_FLAGS := -std=c17 -Wall -Wextra -Wpedantic -g -I$(INCLUDE) -I$(PIGPIO)

.PHONY: clean

$(BIN)/main: $(SRC)/main.c $(BIN)
	$(CC) $(CC_FLAGS) -L$(PIGPIO) $< -o $(BIN)/main -lpigpiod_if2 -lrt -pthread

$(BIN):
	mkdir -p $(BIN)

clean:
	rm $(BIN)/main
