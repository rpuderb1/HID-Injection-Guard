# hid_guard daemon Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2
DEBUG_CFLAGS = -Wall -Wextra -g -DDEBUG
LDFLAGS = -lm
TARGET = hid_guard

# Default build
all: $(TARGET)

$(TARGET): hid_guard.c
	$(CC) $(CFLAGS) -o $(TARGET) hid_guard.c $(LDFLAGS)

# Debug build
debug: hid_guard.c
	$(CC) $(DEBUG_CFLAGS) -o $(TARGET) hid_guard.c $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all debug clean
