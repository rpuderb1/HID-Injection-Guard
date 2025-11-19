# hid_guard daemon Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
DEBUG_CFLAGS = -Wall -Wextra -g -DDEBUG -std=c11
LDFLAGS = -lm
TARGET = hid_guard

# Source files
SRCS = hid_guard.c input_handler.c detector.c
OBJS = $(SRCS:.c=.o)

# Default build
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean $(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

# Install (requires root)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all debug clean install uninstall
