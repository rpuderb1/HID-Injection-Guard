#define _POSIX_C_SOURCE 200809L
#include "input_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <glob.h>
#include <sys/inotify.h>
#include <linux/input-event-codes.h>

#define EVENT_BUF_LEN (10 * (sizeof(struct inotify_event) + 256))
#define DEBUG 0
#define SYSFS_BASE "/sys/kernel/usb_hid_monitor"

// Internal state (static allocation to avoid malloc)
static struct pollfd fds[MAX_DEVICES + 1];
static struct device_info devices[MAX_DEVICES];
static char inotify_buf[EVENT_BUF_LEN];

// Calculate time difference in milliseconds
static double timeval_diff_ms(struct timeval *start_time, struct timeval *end_time) {
    return (end_time->tv_sec - start_time->tv_sec) * 1000.0 +
           (end_time->tv_usec - start_time->tv_usec) / 1000.0;
}

// Read a single sysfs attribute
static int read_sysfs_attr(const char *attr_name, char *buf, size_t buf_size) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", SYSFS_BASE, attr_name);

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    if (fgets(buf, buf_size, f) == NULL) {
        fclose(f);
        return -1;
    }

    // Remove trailing newline
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }

    fclose(f);
    return 0;
}

// Read USB device info from kernel module via sysfs
static void read_device_usb_info(struct device_info *dev) {
    // Initialize to "Unknown" in case sysfs is not available
    strcpy(dev->vid, "Unknown");
    strcpy(dev->pid, "Unknown");
    strcpy(dev->manufacturer, "Unknown");
    strcpy(dev->product, "Unknown");
    strcpy(dev->serial, "Unknown");

    // Attempt to read from sysfs (kernel module may not be loaded)
    read_sysfs_attr("vid", dev->vid, sizeof(dev->vid));
    read_sysfs_attr("pid", dev->pid, sizeof(dev->pid));
    read_sysfs_attr("manufacturer", dev->manufacturer, sizeof(dev->manufacturer));
    read_sysfs_attr("product", dev->product, sizeof(dev->product));
    read_sysfs_attr("serial", dev->serial, sizeof(dev->serial));
}

// Add a new device to monitoring
static int add_device(struct input_state *state, const char *device_path) {
    if (state->num_devices >= MAX_DEVICES) {
        fprintf(stderr, "Maximum device limit reached\n");
        return -1;
    }

    // Open device immediately (race condition: must be fast)
    int fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    // Add to poll set
    state->fds[state->poll_count].fd = fd;
    state->fds[state->poll_count].events = POLLIN;

    // Initialize device info
    state->devices[state->num_devices].path = strdup(device_path);
    gettimeofday(&state->devices[state->num_devices].connected_at, NULL);
    state->devices[state->num_devices].keystroke_count = 0;
    state->devices[state->num_devices].consecutive_errors = 0;

    read_device_usb_info(&state->devices[state->num_devices]);

    // TODO: Future enhancement - Device trust system
    // - Check if VID:PID:Serial seen before in /var/lib/hid_guard/device_history.txt
    // - Calculate trust score based on usage history
    // - Flag first-time devices with higher suspicion

    state->num_devices++;
    state->poll_count++;

    return 0;
}

static int init_inotify(struct input_state *state) {
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("inotify_init1");
        return -1;
    }

    // Watch /dev/input for new device creation
    int watch_fd = inotify_add_watch(inotify_fd, "/dev/input", IN_CREATE);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return -1;
    }

    // Add inotify to poll set (index 0)
    state->fds[0].fd = inotify_fd;
    state->fds[0].events = POLLIN;

    printf("Watching /dev/input for new devices...\n");
    return inotify_fd;
}

// Open all existing /dev/input/event* devices at startup
static int init_existing_devices(struct input_state *state) {
    glob_t globbuf;
    int devices_added = 0;

    // Find all existing /dev/input/event* devices
    if (glob("/dev/input/event*", 0, NULL, &globbuf) == 0) {
        for (size_t glob_idx = 0; glob_idx < globbuf.gl_pathc; glob_idx++) {
            int fd = open(globbuf.gl_pathv[glob_idx], O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                state->fds[state->poll_count].fd = fd;
                state->fds[state->poll_count].events = POLLIN;
                state->devices[state->num_devices].path = strdup(globbuf.gl_pathv[glob_idx]);
                gettimeofday(&state->devices[state->num_devices].connected_at, NULL);
                state->devices[state->num_devices].keystroke_count = 0;
                state->devices[state->num_devices].consecutive_errors = 0;

                // Don't read from kernel module for existing devices (sysfs logs last plugged in device)
                // Initialize to "Unknown" (only new devices get real identification)
                strcpy(state->devices[state->num_devices].vid, "Unknown");
                strcpy(state->devices[state->num_devices].pid, "Unknown");
                strcpy(state->devices[state->num_devices].manufacturer, "Unknown");
                strcpy(state->devices[state->num_devices].product, "Unknown");
                strcpy(state->devices[state->num_devices].serial, "Unknown");

                printf("Monitoring: %s\n", state->devices[state->num_devices].path);
                state->num_devices++;
                state->poll_count++;
                devices_added++;
            }
        }
        globfree(&globbuf);
    }

    return devices_added;
}

int input_init(struct input_state *state) {
    memset(state, 0, sizeof(struct input_state));
    state->fds = fds;
    state->devices = devices;
    state->inotify_buf = inotify_buf;
    state->poll_count = 1;

    printf("HID Guard - Input Monitoring\n");
    printf("==========================================\n");

    // Initialize inotify to watch for new devices
    state->inotify_fd = init_inotify(state);
    if (state->inotify_fd < 0) {
        return -1;
    }

    init_existing_devices(state);

    printf("\nMonitoring %d input device(s), watching for new devices... (Ctrl+C to stop)\n", state->num_devices);
    printf("-------------------------------------------\n\n");

    return 0;
}

// Wait for input events
int input_poll(struct input_state *state) {
    int ret = poll(state->fds, state->poll_count, -1);

    if (ret < 0) {
        if (errno == EINTR) {
            return 0;
        }
        perror("poll error");
        return -1;
    }

    return ret;
}

// Check if there are new devices
int input_has_new_devices(struct input_state *state) {
    return state->fds[0].revents & POLLIN;
}

// Process new device connections
void input_process_new_devices(struct input_state *state) {
    ssize_t len = read(state->inotify_fd, state->inotify_buf, EVENT_BUF_LEN);
    if (len <= 0) {
        return;
    }

    // Parse all inotify events in the buffer
    for (char *ptr = state->inotify_buf; ptr < state->inotify_buf + len;) {
        struct inotify_event *event = (struct inotify_event *)ptr;

        // Only process event* files (input devices)
        if (event->len > 0 && strncmp(event->name, "event", 5) == 0) {
            char device_path[256];
            snprintf(device_path, sizeof(device_path), "/dev/input/%s", event->name);

            if (add_device(state, device_path) == 0) {
                // Get the device info that was just added
                struct device_info *dev = &state->devices[state->num_devices - 1];

                printf("\n[NEW DEVICE] %s\n", device_path);
                printf("  VID:PID = %s:%s | %s - %s\n",
                       dev->vid, dev->pid, dev->manufacturer, dev->product);
                if (strcmp(dev->serial, "Unknown") != 0 && strlen(dev->serial) > 0) {
                    printf("  Serial: %s\n", dev->serial);
                }
                printf("\n");
            }
        }

        ptr += sizeof(struct inotify_event) + event->len;
    }
}

// Read next event from a device
int input_read_event(struct input_state *state, int device_idx, struct input_event *ev) {
    int poll_idx = device_idx + 1;

    // Check for disconnection
    if (state->fds[poll_idx].revents & (POLLERR | POLLHUP)) {
        state->devices[device_idx].consecutive_errors++;

        if (state->devices[device_idx].consecutive_errors <= 3) {
#if DEBUG
            printf("[DEBUG] Ignoring transient error #%d on %s\n",
                   state->devices[device_idx].consecutive_errors,
                   state->devices[device_idx].path);
#endif
            return 0;
        }

        // Real disconnect
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        double duration_sec = timeval_diff_ms(&state->devices[device_idx].connected_at, &current_time) / 1000.0;

        printf("\n[DISCONNECT] %s\n", state->devices[device_idx].path);
        printf("  Device: %s - %s (VID:%s PID:%s)\n",
               state->devices[device_idx].manufacturer,
               state->devices[device_idx].product,
               state->devices[device_idx].vid,
               state->devices[device_idx].pid);
        printf("  Duration: %.2f seconds | Keystrokes: %d\n\n", duration_sec, state->devices[device_idx].keystroke_count);

        close(state->fds[poll_idx].fd);
        free(state->devices[device_idx].path);

        // Shift arrays down
        for (int shift_idx = poll_idx; shift_idx < state->poll_count - 1; shift_idx++) {
            state->fds[shift_idx] = state->fds[shift_idx + 1];
        }
        for (int shift_idx = device_idx; shift_idx < state->num_devices - 1; shift_idx++) {
            state->devices[shift_idx] = state->devices[shift_idx + 1];
        }

        state->poll_count--;
        state->num_devices--;

        return -1;  // Device removed
    }

    // Check if device has data
    if (!(state->fds[poll_idx].revents & POLLIN)) {
        return 0;
    }

    // Read event
    ssize_t bytes = read(state->fds[poll_idx].fd, ev, sizeof(struct input_event));
    if (bytes != sizeof(struct input_event)) {
        return 0;
    }

    state->devices[device_idx].consecutive_errors = 0;
    return 1;
}

// Get device info
struct device_info* input_get_device_info(struct input_state *state, int device_idx) {
    return &state->devices[device_idx];
}

const char* input_keycode_name(int code) {
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
        case KEY_ENTER: return "ENTER";
        case KEY_SPACE: return "SPACE";
        case KEY_BACKSPACE: return "BACKSPACE";
        case KEY_TAB: return "TAB";
        case KEY_ESC: return "ESC";
        case KEY_DELETE: return "DELETE";
        case KEY_CAPSLOCK: return "CAPSLOCK";
        case KEY_LEFTCTRL: return "LEFTCTRL";
        case KEY_RIGHTCTRL: return "RIGHTCTRL";
        case KEY_LEFTALT: return "LEFTALT";
        case KEY_RIGHTALT: return "RIGHTALT";
        case KEY_LEFTSHIFT: return "LEFTSHIFT";
        case KEY_RIGHTSHIFT: return "RIGHTSHIFT";
        case KEY_LEFTMETA: return "SUPER";
        case KEY_RIGHTMETA: return "SUPER";
        case KEY_UP: return "UP";
        case KEY_DOWN: return "DOWN";
        case KEY_LEFT: return "LEFT";
        case KEY_RIGHT: return "RIGHT";
        case KEY_HOME: return "HOME";
        case KEY_END: return "END";
        case KEY_PAGEUP: return "PAGEUP";
        case KEY_PAGEDOWN: return "PAGEDOWN";
        case KEY_INSERT: return "INSERT";
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

// Convert keycode to printable character
char input_keycode_to_char(int code, int shift_active) {
    // Letters
    if (!shift_active) {
        switch(code) {
            case KEY_Q: return 'q'; case KEY_W: return 'w'; case KEY_E: return 'e';
            case KEY_R: return 'r'; case KEY_T: return 't'; case KEY_Y: return 'y';
            case KEY_U: return 'u'; case KEY_I: return 'i'; case KEY_O: return 'o';
            case KEY_P: return 'p'; case KEY_A: return 'a'; case KEY_S: return 's';
            case KEY_D: return 'd'; case KEY_F: return 'f'; case KEY_G: return 'g';
            case KEY_H: return 'h'; case KEY_J: return 'j'; case KEY_K: return 'k';
            case KEY_L: return 'l'; case KEY_Z: return 'z'; case KEY_X: return 'x';
            case KEY_C: return 'c'; case KEY_V: return 'v'; case KEY_B: return 'b';
            case KEY_N: return 'n'; case KEY_M: return 'm';
        }
    } else {
        switch(code) {
            case KEY_Q: return 'Q'; case KEY_W: return 'W'; case KEY_E: return 'E';
            case KEY_R: return 'R'; case KEY_T: return 'T'; case KEY_Y: return 'Y';
            case KEY_U: return 'U'; case KEY_I: return 'I'; case KEY_O: return 'O';
            case KEY_P: return 'P'; case KEY_A: return 'A'; case KEY_S: return 'S';
            case KEY_D: return 'D'; case KEY_F: return 'F'; case KEY_G: return 'G';
            case KEY_H: return 'H'; case KEY_J: return 'J'; case KEY_K: return 'K';
            case KEY_L: return 'L'; case KEY_Z: return 'Z'; case KEY_X: return 'X';
            case KEY_C: return 'C'; case KEY_V: return 'V'; case KEY_B: return 'B';
            case KEY_N: return 'N'; case KEY_M: return 'M';
        }
    }

    // Numbers and symbols
    if (!shift_active) {
        switch(code) {
            case KEY_1: return '1'; case KEY_2: return '2'; case KEY_3: return '3';
            case KEY_4: return '4'; case KEY_5: return '5'; case KEY_6: return '6';
            case KEY_7: return '7'; case KEY_8: return '8'; case KEY_9: return '9';
            case KEY_0: return '0';
            case KEY_SPACE: return ' '; case KEY_MINUS: return '-'; case KEY_EQUAL: return '=';
            case KEY_LEFTBRACE: return '['; case KEY_RIGHTBRACE: return ']';
            case KEY_SEMICOLON: return ';'; case KEY_APOSTROPHE: return '\'';
            case KEY_GRAVE: return '`'; case KEY_BACKSLASH: return '\\';
            case KEY_COMMA: return ','; case KEY_DOT: return '.'; case KEY_SLASH: return '/';
        }
    } else {
        switch(code) {
            case KEY_1: return '!'; case KEY_2: return '@'; case KEY_3: return '#';
            case KEY_4: return '$'; case KEY_5: return '%'; case KEY_6: return '^';
            case KEY_7: return '&'; case KEY_8: return '*'; case KEY_9: return '(';
            case KEY_0: return ')';
            case KEY_SPACE: return ' '; case KEY_MINUS: return '_'; case KEY_EQUAL: return '+';
            case KEY_LEFTBRACE: return '{'; case KEY_RIGHTBRACE: return '}';
            case KEY_SEMICOLON: return ':'; case KEY_APOSTROPHE: return '"';
            case KEY_GRAVE: return '~'; case KEY_BACKSLASH: return '|';
            case KEY_COMMA: return '<'; case KEY_DOT: return '>'; case KEY_SLASH: return '?';
        }
    }

    return '\0';  // Non-printable key
}

// Check if event is a key press
int input_is_key_press(struct input_event *ev) {
    return ev->type == EV_KEY && ev->value == 1;
}

// Cleanup
void input_cleanup(struct input_state *state) {
    close(state->inotify_fd);
    for (int fd_idx = 1; fd_idx < state->poll_count; fd_idx++) {
        close(state->fds[fd_idx].fd);
    }
    for (int device_idx = 0; device_idx < state->num_devices; device_idx++) {
        free(state->devices[device_idx].path);
    }
}
