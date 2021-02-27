INCLUDE     := ./include
SRC         := ./src
OBJ         := ./obj
BIN         := ./bin
MERCURY_API := ./lib/mercuryapi-1.31.4.35/c/src/api

SRC_FILES   := $(wildcard $(SRC)/*.c)
SRC_OBJS    := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRC_FILES))

CC          := gcc
CC_FLAGS    := -std=c17 -Wall -Wextra -Wpedantic -g -I$(INCLUDE) -I$(MERCURY_API)


.PHONY: clean directories main

main: directories $(MERCURY)/libmercuryapi.so.1 $(BIN)/main

$(BIN)/main: $(SRC_OBJS)
	$(CC) $(CC_FLAGS) -D_POSIX_C_SOURCE -L$(MERCURY_API) $^ -o $(BIN)/main -lmercuryapi -lpthread

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@

$(MERCURY)/libmercuryapi.so.1:
	cd $(MERCURY_API) && $(MAKE)

directories: $(BIN) $(OBJ)

$(BIN):
	mkdir -p $(BIN)

$(OBJ):
	mkdir -p $(OBJ)

clean:
	rm $(BIN)/main
	rm $(OBJ)/*.o
	cd $(MERCURY_API) && $(MAKE) clean
