#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <linux/input.h>
#include <sys/time.h>

#define MAX_DEVICES 32

/**
 * Input Handler Module
 * Handles all input device management and event reading
 */

#define IKT_BUFFER_SIZE 100

struct device_info {
    char *path;
    struct timeval connected_at;
    int keystroke_count;
    int consecutive_errors;

    // USB device identification (from kernel module via sysfs)
    char vid[8];
    char pid[8];
    char manufacturer[64];
    char product[64];
    char serial[64];

    // Per-device timing tracking
    struct timeval last_key_time;
    int has_last_key;
    double ikt_buffer[IKT_BUFFER_SIZE];
    int ikt_count;
    int ikt_index;
    int total_keys;
};

struct input_state {
    struct pollfd *fds;
    struct device_info *devices;
    int num_devices;
    int poll_count;
    int inotify_fd;
    char *inotify_buf;
};

// IOpens existing devices, sets up inotify
int input_init(struct input_state *state);

// Wait for input events (blocks until activity)
int input_poll(struct input_state *state);

int input_has_new_devices(struct input_state *state);

void input_process_new_devices(struct input_state *state);

// Read next event from device (returns 1 if event read, 0 if none, -1 on error)
int input_read_event(struct input_state *state, int device_idx, struct input_event *ev);

struct device_info* input_get_device_info(struct input_state *state, int device_idx);

// Convert keycode to human-readable name
const char* input_keycode_name(int code);

// Convert keycode to printable character
// Returns '\0' for non-printable keys
char input_keycode_to_char(int code, int shift_active);

int input_is_key_press(struct input_event *ev);

void input_cleanup(struct input_state *state);

#endif
