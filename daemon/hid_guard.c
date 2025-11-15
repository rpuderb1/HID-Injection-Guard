#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <errno.h>

#define INPUT_DEVICE "/dev/input/event0"  // Todo: Make configureable

// Map keycode to key name
const char* keycode_to_name(int code) {
    switch(code) {
        case KEY_Q: return "Q";
        case KEY_W: return "W";
        case KEY_E: return "E";
        case KEY_R: return "R";
        case KEY_T: return "T";
        case KEY_Y: return "Y";
        case KEY_U: return "U";
        case KEY_I: return "I";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_A: return "A";
        case KEY_S: return "S";
        case KEY_D: return "D";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_Z: return "Z";
        case KEY_X: return "X";
        case KEY_C: return "C";
        case KEY_V: return "V";
        case KEY_B: return "B";
        case KEY_N: return "N";
        case KEY_M: return "M";

        // Numbers
        case KEY_1: return "1";
        case KEY_2: return "2";
        case KEY_3: return "3";
        case KEY_4: return "4";
        case KEY_5: return "5";
        case KEY_6: return "6";
        case KEY_7: return "7";
        case KEY_8: return "8";
        case KEY_9: return "9";
        case KEY_0: return "0";

        // Control keys
        case KEY_ENTER: return "ENTER";
        case KEY_SPACE: return "SPACE";
        case KEY_BACKSPACE: return "BACKSPACE";
        case KEY_TAB: return "TAB";
        case KEY_ESC: return "ESC";
        case KEY_DELETE: return "DELETE";
        case KEY_CAPSLOCK: return "CAPSLOCK";

        // Modifier keys
        case KEY_LEFTCTRL: return "LEFTCTRL";
        case KEY_RIGHTCTRL: return "RIGHTCTRL";
        case KEY_LEFTALT: return "LEFTALT";
        case KEY_RIGHTALT: return "RIGHTALT";
        case KEY_LEFTSHIFT: return "LEFTSHIFT";
        case KEY_RIGHTSHIFT: return "RIGHTSHIFT";
        case KEY_LEFTMETA: return "SUPER";
        case KEY_RIGHTMETA: return "SUPER";

        // Arrow keys
        case KEY_UP: return "UP";
        case KEY_DOWN: return "DOWN";
        case KEY_LEFT: return "LEFT";
        case KEY_RIGHT: return "RIGHT";

        // Navigation keys
        case KEY_HOME: return "HOME";
        case KEY_END: return "END";
        case KEY_PAGEUP: return "PAGEUP";
        case KEY_PAGEDOWN: return "PAGEDOWN";
        case KEY_INSERT: return "INSERT";

        // Function keys
        case KEY_F1: return "F1";
        case KEY_F2: return "F2";
        case KEY_F3: return "F3";
        case KEY_F4: return "F4";
        case KEY_F5: return "F5";
        case KEY_F6: return "F6";
        case KEY_F7: return "F7";
        case KEY_F8: return "F8";
        case KEY_F9: return "F9";
        case KEY_F10: return "F10";
        case KEY_F11: return "F11";
        case KEY_F12: return "F12";

        // Symbols
        case KEY_MINUS: return "-";
        case KEY_EQUAL: return "=";
        case KEY_LEFTBRACE: return "[";
        case KEY_RIGHTBRACE: return "]";
        case KEY_SEMICOLON: return ";";
        case KEY_APOSTROPHE: return "'";
        case KEY_GRAVE: return "`";
        case KEY_BACKSLASH: return "\\";
        case KEY_COMMA: return ",";
        case KEY_DOT: return ".";
        case KEY_SLASH: return "/";

        default: return "UNKNOWN";
    }
}

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

        // Only process keyboard events (type=1)
        if (ev.type != EV_KEY) {
            continue;
        }

        // Only show key presses (value=1), ignore releases (value=0) and repeats (value=2)
        if (ev.value != 1) {
            continue;
        }

        // Convert keycode to readable name
        const char* key_name = keycode_to_name(ev.code);

        // Show keycode for unknown keys
        if (strcmp(key_name, "UNKNOWN") == 0) {
            printf("Key pressed: UNKNOWN (code=%d)\n", ev.code);
        } else {
            printf("Key pressed: %s\n", key_name);
        }
    }

    close(fd);
    return 0;
}
