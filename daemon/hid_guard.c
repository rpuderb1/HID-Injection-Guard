#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <poll.h>
#include <glob.h>
#include <sys/inotify.h>

#define MAX_DEVICES 32
#define IKT_BUFFER_SIZE 100 // Track last 100 inter-keystroke timings
#define STATS_INTERVAL 10 // Show statistics every N keystrokes
#define EVENT_BUF_LEN (10 * (sizeof(struct inotify_event) + 256))
#define DEBUG 0 // Set to 1 to enable debug logging

struct device_info {
    char *path;
    struct timeval connected_at;
    int keystroke_count;
    int consecutive_errors;  // Track repeated errors to detect real disconnects
};

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

// Calculate time difference in milliseconds
double timeval_diff_ms(struct timeval *start_time, struct timeval *end_time) {
    return (end_time->tv_sec - start_time->tv_sec) * 1000.0 +
           (end_time->tv_usec - start_time->tv_usec) / 1000.0;
}

// Calculate mean of IKT values
double calculate_mean(double *values, int count) {
    if (count == 0) return 0.0;

    double sum = 0.0;
    for (int value_idx = 0; value_idx < count; value_idx++) {
        sum += values[value_idx];
    }
    return sum / count;
}

// Calculate jitter (standard deviation) of IKT values
double calculate_jitter(double *values, int count, double mean) {
    if (count < 2) return 0.0;

    double variance = 0.0;
    for (int value_idx = 0; value_idx < count; value_idx++) {
        double diff = values[value_idx] - mean;
        variance += diff * diff;
    }
    variance /= (count - 1);  // Sample variance
    return sqrt(variance);
}

// Display timing statistics
void display_statistics(double *ikt_buffer, int ikt_count, int total_keys) {
    double mean = calculate_mean(ikt_buffer, ikt_count);
    double jitter = calculate_jitter(ikt_buffer, ikt_count, mean);

    printf("\n");
    printf("========== TIMING STATISTICS (last %d keystrokes) ==========\n", ikt_count);
    printf("  Mean IKT:    %7.2f ms\n", mean);
    printf("  Jitter (SD): %7.2f ms\n", jitter);
    printf("  Total keys:  %d\n", total_keys);
    printf("===========================================================\n\n");
}

// Add a new device to monitoring
int add_device(const char *device_path, struct pollfd *fds, struct device_info *devices, int *num_devices, int *poll_count) {
    if (*num_devices >= MAX_DEVICES) {
        fprintf(stderr, "Maximum device limit reached\n");
        return -1;
    }

    // Open device immediately (race condition: must be fast)
    int fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    // Add to poll set
    fds[*poll_count].fd = fd;
    fds[*poll_count].events = POLLIN;

    // Initialize device info
    devices[*num_devices].path = strdup(device_path);
    gettimeofday(&devices[*num_devices].connected_at, NULL);
    devices[*num_devices].keystroke_count = 0;
    devices[*num_devices].consecutive_errors = 0;

    (*num_devices)++;
    (*poll_count)++;

    return 0;
}

// Remove a disconnected device from monitoring
void remove_device(int poll_idx, struct pollfd *fds, struct device_info *devices, int *num_devices, int *poll_count) {
    int device_idx = poll_idx - 1;  // Map poll index to device index

    // Calculate and display connection duration
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    double duration_sec = timeval_diff_ms(&devices[device_idx].connected_at, &current_time) / 1000.0;

    printf("\n[DISCONNECT] Device removed: %s\n", devices[device_idx].path);
    printf("  Duration: %.2f seconds | Keystrokes: %d\n\n", duration_sec, devices[device_idx].keystroke_count);

    // Close file descriptor and free memory
    close(fds[poll_idx].fd);
    free(devices[device_idx].path);

    // Remove from arrays by shifting everything down
    for (int shift_idx = poll_idx; shift_idx < *poll_count - 1; shift_idx++) {
        fds[shift_idx] = fds[shift_idx + 1];
    }
    for (int shift_idx = device_idx; shift_idx < *num_devices - 1; shift_idx++) {
        devices[shift_idx] = devices[shift_idx + 1];
    }

    (*poll_count)--;
    (*num_devices)--;
}

// Initialize inotify to watch for new input devices
int init_inotify(struct pollfd *fds) {
    // Create inotify instance
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
    fds[0].fd = inotify_fd;
    fds[0].events = POLLIN;

    printf("Watching /dev/input for new devices...\n");
    return inotify_fd;
}

// Open all existing /dev/input/event* devices at startup
int init_existing_devices(struct pollfd *fds, struct device_info *devices, int *num_devices, int *poll_count) {
    glob_t globbuf;
    int devices_added = 0;

    // Find all existing /dev/input/event* devices
    if (glob("/dev/input/event*", 0, NULL, &globbuf) == 0) {
        for (size_t glob_idx = 0; glob_idx < globbuf.gl_pathc; glob_idx++) {
            int fd = open(globbuf.gl_pathv[glob_idx], O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                fds[*poll_count].fd = fd;
                fds[*poll_count].events = POLLIN;
                devices[*num_devices].path = strdup(globbuf.gl_pathv[glob_idx]);
                gettimeofday(&devices[*num_devices].connected_at, NULL);
                devices[*num_devices].keystroke_count = 0;
                devices[*num_devices].consecutive_errors = 0;
                printf("Monitoring: %s\n", devices[*num_devices].path);
                (*num_devices)++;
                (*poll_count)++;
                devices_added++;
            }
        }
        globfree(&globbuf);
    }

    return devices_added;
}

// Process inotify events and add newly created input devices
// This handles dynamic device detection when new devices are plugged in
void process_inotify_events(int inotify_fd, char *inotify_buf, struct pollfd *fds,
                            struct device_info *devices, int *num_devices, int *poll_count) {
    ssize_t len = read(inotify_fd, inotify_buf, EVENT_BUF_LEN);
    if (len <= 0) {
        return;
    }

    // Parse all inotify events in the buffer
    for (char *ptr = inotify_buf; ptr < inotify_buf + len;) {
        struct inotify_event *event = (struct inotify_event *)ptr;

        // Only process event* files (input devices)
        if (event->len > 0 && strncmp(event->name, "event", 5) == 0) {
            char device_path[256];
            snprintf(device_path, sizeof(device_path), "/dev/input/%s", event->name);

            if (add_device(device_path, fds, devices, num_devices, poll_count) == 0) {
                printf("\n[NEW DEVICE] Now monitoring: %s\n\n", device_path);
            }
        }

        ptr += sizeof(struct inotify_event) + event->len;
    }
}

// Calculates timing metrics and displays keystroke information
void process_keystroke(struct input_event *ev, struct device_info *devices, int device_idx,
                       struct timeval *last_key_time, int *has_last_key,
                       double *ikt_buffer, int *ikt_count, int *ikt_index, int *total_keys) {
    // Only process keyboard events (type=1)
    if (ev->type != EV_KEY) {
        return;
    }

    // Only show key presses (value=1), ignore releases (value=0) and repeats (value=2)
    if (ev->value != 1) {
        return;
    }

    // Convert keycode to readable name
    const char* key_name = keycode_to_name(ev->code);

    // Calculate IKT (Inter-Keystroke Timing)
    double ikt_ms = 0.0;
    if (*has_last_key) {
        ikt_ms = timeval_diff_ms(last_key_time, &ev->time);

        // Store IKT in circular buffer
        ikt_buffer[*ikt_index] = ikt_ms;
        *ikt_index = (*ikt_index + 1) % IKT_BUFFER_SIZE;
        if (*ikt_count < IKT_BUFFER_SIZE) {
            (*ikt_count)++;
        }
    }

    // Update last key timestamp
    *last_key_time = ev->time;
    *has_last_key = 1;
    (*total_keys)++;
    devices[device_idx].keystroke_count++;
    devices[device_idx].consecutive_errors = 0;  // Reset error counter on successful read

    // Display key press with timing info and source device
    printf("[%s] ", devices[device_idx].path);
    if (strcmp(key_name, "UNKNOWN") == 0) {
        printf("Key: UNKNOWN (code=%d)", ev->code);
    } else {
        printf("Key: %-12s", key_name);
    }

    // Show IKT for this keystroke
    if (ikt_ms > 0) {
        printf(" | IKT: %7.2f ms", ikt_ms);
    }
    printf("\n");

    // Display statistics every STATS_INTERVAL keystrokes
    if (*total_keys % STATS_INTERVAL == 0 && *ikt_count > 0) {
        display_statistics(ikt_buffer, *ikt_count, *total_keys);
    }
}

int main(int argc, char *argv[]) {
    struct pollfd fds[MAX_DEVICES + 1];  // +1 for inotify
    struct device_info devices[MAX_DEVICES];
    int num_devices = 0;
    struct input_event ev;
    ssize_t bytes;
    int inotify_fd;
    char inotify_buf[EVENT_BUF_LEN];
    int poll_count = 1;  // Start with inotify at index 0

    // Timing tracking variables
    struct timeval last_key_time = {0, 0};
    int has_last_key = 0;
    double ikt_buffer[IKT_BUFFER_SIZE];
    int ikt_count = 0;
    int ikt_index = 0;
    int total_keys = 0;

    printf("HID Guard - Input Monitoring\n");
    printf("==========================================\n");

    // Initialize inotify to watch for new devices
    inotify_fd = init_inotify(fds);
    if (inotify_fd < 0) {
        return 1;
    }

    // Find and open all existing /dev/input/event* devices
    init_existing_devices(fds, devices, &num_devices, &poll_count);

    printf("\nMonitoring %d input device(s), watching for new devices... (Ctrl+C to stop)\n", num_devices);
    printf("-------------------------------------------\n\n");

    // Main event loop - poll inotify + all input devices
    while (1) {
        int ret = poll(fds, poll_count, -1);  // Wait indefinitely for events

        if (ret < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, try again
            }
            perror("poll error");
            break;
        }

        // Check for inotify events (new devices)
        if (fds[0].revents & POLLIN) {
            process_inotify_events(inotify_fd, inotify_buf, fds, devices, &num_devices, &poll_count);
        }

        // Check input devices for events (indices 1+)
        for (int poll_idx = 1; poll_idx < poll_count; poll_idx++) {
            int device_idx = poll_idx - 1;  // Map poll index to device_names index

            // Check for disconnection (POLLERR | POLLHUP)
            if (fds[poll_idx].revents & (POLLERR | POLLHUP)) {
                // Increment consecutive error counter
                devices[device_idx].consecutive_errors++;

                // Ignore transient errors during initialization (first few errors)
                // After 3 consecutive errors, assume it's a real disconnect
                if (devices[device_idx].consecutive_errors <= 3) {
#if DEBUG
                    printf("[DEBUG] Ignoring transient error #%d on %s - keeping in poll\n",
                           devices[device_idx].consecutive_errors, devices[device_idx].path);
#endif
                    continue;
                }

                // Multiple consecutive errors - this is a real disconnect
                remove_device(poll_idx, fds, devices, &num_devices, &poll_count);
                poll_idx--;  // Re-check this index since we shifted
                continue;
            }

            if (!(fds[poll_idx].revents & POLLIN)) {
                continue;  // No data on this device
            }

            // Read event from this device
            while ((bytes = read(fds[poll_idx].fd, &ev, sizeof(struct input_event))) > 0) {
                if (bytes < (ssize_t) sizeof(struct input_event)) {
                    break;
                }

                // Process keystroke event (filtering, timing, display)
                process_keystroke(&ev, devices, device_idx, &last_key_time, &has_last_key,
                                ikt_buffer, &ikt_count, &ikt_index, &total_keys);
            }
        }
    }

    // Cleanup
    close(inotify_fd);
    for (int fd_idx = 1; fd_idx < poll_count; fd_idx++) {
        close(fds[fd_idx].fd);
    }
    for (int device_idx = 0; device_idx < num_devices; device_idx++) {
        free(devices[device_idx].path);
    }
    return 0;
}
