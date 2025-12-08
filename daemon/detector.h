#ifndef DETECTOR_H
#define DETECTOR_H

#include <linux/input.h>
#include <sys/time.h>

#define CMD_BUFFER_SIZE 4096

struct device_info;

/**
 * Detector Module
 * Handles keystroke analysis and attack detection:
 * - Timing analysis (IKT, jitter) [per-device]
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
    // Command buffering
    char cmd_buffer[CMD_BUFFER_SIZE];
    int cmd_length;
    int shift_pressed;
    int caps_lock_active;
    int backspace_count;

    // Pattern matching & scoring
    int pattern_download; // curl, wget detected
    int pattern_piped_download; // download piped to shell
    int pattern_base64; // base64 decode operations
    int pattern_eval; // eval with command substitution
    int pattern_reverse_shell; // reverse shell attempts
    int pattern_execution; // chmod +x, ./ execution
    int pattern_persistence; // crontab, bashrc, profile
    int pattern_command_chain; // multiple pipes >3
    int pattern_fast_timing; // automated timing detected
    int current_score; // weighted score for current session

    // TODO: Attack chain detection
    // - Track command history with timestamps
    // - Detect Download→Execute sequences
    // - Detect full attack chains (Download→Execute→Persist)

    // TODO: Composite scoring & alerts
    // - Add syslog integration
};

void detector_init(struct detector_state *state);

// Process keystroke for command buffering and returns IKT
double detector_process_keystroke(struct detector_state *state, struct device_info *device, struct input_event *ev);

// Get timing statistics for specific device
struct timing_stats detector_get_stats(struct device_info *device);

void detector_analyze_command(struct detector_state *state, struct device_info *device, const char *command);

#endif
