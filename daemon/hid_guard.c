#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>

#define INPUT_DEVICE "/dev/input/event0"  // Todo: Make configureable

int main(int argc, char *argv[]) {
    int fd;
    struct input_event ev;
    ssize_t bytes;

    printf("HID Guard - Input Monitoring\n");
    printf("==========================================\n");
    printf("Opening input device: %s\n", INPUT_DEVICE);

    // Open input device
    fd = open(INPUT_DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Error opening input device");
        printf("\nTroubleshooting:\n");
        printf("  1. Run with sudo: sudo ./hid_guard\n");
        printf("  2. Check available devices: ls -l /dev/input/event*\n");
        printf("  3. Find keyboard device: cat /proc/bus/input/devices\n");
        return 1;
    }

    printf("Successfully opened %s\n", INPUT_DEVICE);
    printf("Monitoring input events... (Ctrl+C to stop)\n");
    printf("-------------------------------------------\n\n");

    // Main event loop (read and print raw events)
    while (1) {
        bytes = read(fd, &ev, sizeof(struct input_event));

        if (bytes < (ssize_t) sizeof(struct input_event)) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, try again
            }
            perror("Error reading input event");
            break;
        }

        // Print raw event data:
        // time = unix timestamp (seconds.microseconds)
        // type = event type (0=sync, 1=key, 2=relative, 3=absolute, 4=misc)
        // code = specific event code (Ex: 30=KEY_A for type=1 keyboard events)
        // value = meaning depends on type:
        //   - type=1 (key): 0=release, 1=press, 2=repeat
        //   - type=4 (misc): raw hardware scancode
        //   - type=0 (sync): 0
        // TODO: Filter to only show type=1 (EV_KEY) events
        printf("Event: time=%ld.%06ld, type=%d, code=%d, value=%d\n",
               ev.time.tv_sec, ev.time.tv_usec,
               ev.type, ev.code, ev.value);
    }

    close(fd);
    return 0;
}
