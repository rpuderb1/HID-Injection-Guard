CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
DAEMON_DIR = ../daemon

# Test executable
TEST_DETECTOR = test_detector

# Object files needed
DETECTOR_OBJ = detector.o input_handler.o

all: $(TEST_DETECTOR)

# Build test executable
$(TEST_DETECTOR): test_detector.c $(DETECTOR_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_DETECTOR) test_detector.c $(DETECTOR_OBJ) -lm

# Build detector.o from daemon directory
detector.o: $(DAEMON_DIR)/detector.c $(DAEMON_DIR)/detector.h
	$(CC) $(CFLAGS) -c $(DAEMON_DIR)/detector.c -o detector.o

# Build input_handler.o from daemon directory (needed for keycode functions)
input_handler.o: $(DAEMON_DIR)/input_handler.c $(DAEMON_DIR)/input_handler.h
	$(CC) $(CFLAGS) -c $(DAEMON_DIR)/input_handler.c -o input_handler.o

test: $(TEST_DETECTOR)
	@echo "Running HID Guard tests..."
	@./$(TEST_DETECTOR)

clean:
	rm -f $(TEST_DETECTOR) *.o

.PHONY: all clean test
