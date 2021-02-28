INCLUDE       := ./include
SRC           := ./src
OBJ           := ./obj
BIN           := ./bin
MERCURY_API   := ./lib/mercuryapi-1.31.4.35/c/src/api

SRC_FILES     := $(wildcard $(SRC)/*.c)
DEBUG_OBJS    := $(patsubst $(SRC)/%.c, $(OBJ)/%_debug.o, $(SRC_FILES))
RELEASE_OBJS  := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRC_FILES))

CC            := gcc
CC_FLAGS      := -std=gnu17 -Wall -Wextra -Wpedantic -I$(INCLUDE) -I$(MERCURY_API)
DEBUG_FLAGS   := $(CC_FLAGS) -g -DDEBUG
RELEASE_FLAGS := $(CC_FLAGS) -O3


.PHONY: clean directories debug release

debug: directories $(MERCURY)/libmercuryapi.so.1 $(BIN)/debug_main

release: directories $(MERCURY)/libmercuryapi.so.1 $(BIN)/release_main

$(BIN)/debug_main: $(DEBUG_OBJS)
	$(CC) $(DEBUG_FLAGS) -L$(MERCURY_API) $^  -lmercuryapi -lpthread -o $@

$(BIN)/release_main: $(RELEASE_OBJS)
	$(CC) $(RELEASE_FLAGS) -L$(MERCURY_API) $^  -lmercuryapi -lpthread -o $@

$(OBJ)/%_debug.o: $(SRC)/%.c
	$(CC) $(DEBUG_FLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(RELEASE_FLAGS) -c $< -o $@

$(MERCURY)/libmercuryapi.so.1:
	cd $(MERCURY_API) && $(MAKE)

directories: $(BIN) $(OBJ)

$(BIN):
	mkdir -p $(BIN)

$(OBJ):
	mkdir -p $(OBJ)

clean:
	rm $(BIN)/debug_main
	rm $(BIN)/release_main
	rm $(OBJ)/*.o
	cd $(MERCURY_API) && $(MAKE) clean
