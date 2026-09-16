CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRC = src/main.c src/telemetria.c
BIN = safedrive

all: $(BIN)

$(BIN): $(SRC) src/telemetria.h
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

run: all
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean
