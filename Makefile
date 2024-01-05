CC=gcc
CFLAGS=-Wall -g
TARGET=main

.PHONY: all
all: $(TARGET)

.PHONY: $(TARGET)
build: $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

.PHONY: clean
clean:
	rm -rf $(TARGET) *.o
