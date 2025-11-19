#ifndef DETECTOR_H
#define DETECTOR_H

#include <linux/input.h>
#include <sys/time.h>

#define CMD_BUFFER_SIZE 4096
#define IKT_BUFFER_SIZE 100
#define STATS_INTERVAL 10

/**
 * Detector Module
 * Handles keystroke analysis and attack detection:
 * - Timing analysis (IKT, jitter)
 * - Command reconstruction
 * - Pattern matching
 * - Scoring and alerts
 */

struct timing_stats {
    double mean_ikt;
    double jitter;
    int sample_count;
    int total_keys;
};

struct detector_state {
    // Timing analysis
    struct timeval last_key_time;
    int has_last_key;
    double ikt_buffer[IKT_BUFFER_SIZE];
    int ikt_count;
    int ikt_index;
    int total_keys;

    // Command buffering
    char cmd_buffer[CMD_BUFFER_SIZE];
    int cmd_length;
    int shift_pressed;
    int caps_lock_active;

    // TODO: Pattern matching (context-aware weighted scoring)
    // - Download commands: curl, wget (if piped to sh/bash [increase score])
    // - Obfuscation patterns:
    //   - base64 + decode operations
    //   - eval with command substitution $(curl...)
    //   - exec with /dev/tcp/ or bash -i (reverse shell)
    // - Execution patterns: chmod +x followed by ./
    // - Persistence: crontab modifications, .bashrc/.profile edits
    // - Command chaining: Multiple pipes >3

    // TODO: Attack chain detection
    // - Track command history with timestamps
    // - Detect Download→Execute sequences
    // - Detect full attack chains (Download→Execute→Persist)

    // TODO: Composite scoring & alerts
    // - Implement weighted scoring system
    // - Set alert thresholds
    // - Add syslog integration
};

void detector_init(struct detector_state *state);

// Returns IKT for keystroke, or 0 if first key
double detector_process_keystroke(struct detector_state *state, struct input_event *ev);

// Get current timing statistics
struct timing_stats detector_get_stats(struct detector_state *state);

// Check if it's time to display periodic statistics
int detector_should_show_stats(struct detector_state *state);

// Display timing statistics
void detector_display_stats(struct detector_state *state);

// Get the last completed command
// Returns NULL if no command completed
const char* detector_get_completed_command(struct detector_state *state);

#endif
