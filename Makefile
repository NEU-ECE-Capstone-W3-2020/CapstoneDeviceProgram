INCLUDE  := ./include
SRC      := ./src
BIN      := ./bin

PIGPIOD := /home/neil/school/capstone/pigpio

CC       := gcc
CC_FLAGS := -std=c11 -Wall -Wextra -Wpedantic -g -I$(INCLUDE) -I$(PIGPIOD)

.PHONY: clean all

all: $(BIN)/main

$(BIN)/main: $(SRC)/main.c $(BIN)
	$(CC) $(CC_FLAGS) -pthread $< -o $(BIN)/main -L$(PIGPIOD) -lpigpiod_if2 -lrt

$(BIN):
	mkdir -p $(BIN)

clean:
	rm $(BIN)/main
