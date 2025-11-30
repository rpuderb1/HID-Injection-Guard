#include "detector.h"
#include "input_handler.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <linux/input-event-codes.h>

#ifndef IKT_BUFFER_SIZE
#define IKT_BUFFER_SIZE 100
#endif

// ===== TIMING ANALYSIS =====

static double timeval_diff_ms(struct timeval *start_time, struct timeval *end_time) {
    return (end_time->tv_sec - start_time->tv_sec) * 1000.0 +
           (end_time->tv_usec - start_time->tv_usec) / 1000.0;
}

// Calculate mean of IKT values
static double calculate_mean(double *values, int count) {
    if (count == 0) return 0.0;

    double sum = 0.0;
    for (int value_idx = 0; value_idx < count; value_idx++) {
        sum += values[value_idx];
    }
    return sum / count;
}

// Calculate jitter (standard deviation) of IKT values
static double calculate_jitter(double *values, int count, double mean) {
    if (count < 2) return 0.0;

    double variance = 0.0;
    for (int value_idx = 0; value_idx < count; value_idx++) {
        double diff = values[value_idx] - mean;
        variance += diff * diff;
    }
    variance /= (count - 1); // Sample variance
    return sqrt(variance);
}

// ===== COMMAND BUFFERING =====

void detector_init(struct detector_state *state) {
    memset(state, 0, sizeof(struct detector_state));
}

// Command for analysis (cleared buffer on Enter)
static char saved_command[CMD_BUFFER_SIZE];

double detector_process_keystroke(struct detector_state *state, struct device_info *device, struct input_event *ev) {
    double ikt_ms = 0.0;

    // Track shift state changes (including releases)
    if (ev->code == KEY_LEFTSHIFT || ev->code == KEY_RIGHTSHIFT) {
        state->shift_pressed = (ev->value == 1);
        return 0.0;
    }

    // Track caps lock toggle (only on press)
    if (ev->code == KEY_CAPSLOCK && ev->value == 1) {
        state->caps_lock_active = !state->caps_lock_active;
        return 0.0;
    }

    // Only process key presses for timing and buffering
    if (ev->type != EV_KEY || ev->value != 1) {
        return 0.0;
    }

    // Calculate IKT if we have a previous keystroke
    if (device->has_last_key) {
        ikt_ms = timeval_diff_ms(&device->last_key_time, &ev->time);

        // Store in device's circular buffer
        device->ikt_buffer[device->ikt_index] = ikt_ms;
        device->ikt_index = (device->ikt_index + 1) % IKT_BUFFER_SIZE;
        if (device->ikt_count < IKT_BUFFER_SIZE) {
            device->ikt_count++;
        }
    }

    device->last_key_time = ev->time;
    device->has_last_key = 1;
    device->total_keys++;

    // Handle command buffering
    int shift_active = state->shift_pressed || state->caps_lock_active;

    if (ev->code == KEY_ENTER) {
        // Command completed
        if (state->cmd_length > 0) {
            // Save command before clearing
            strncpy(saved_command, state->cmd_buffer, CMD_BUFFER_SIZE - 1);
            saved_command[CMD_BUFFER_SIZE - 1] = '\0';

            printf("\n"
                   "╔════════════════════════════════════════════════════════════════\n"
                   "║ COMMAND: %s\n"
                   "╚════════════════════════════════════════════════════════════════\n\n",
                   state->cmd_buffer);

            detector_analyze_command(state, device, saved_command);

            // Display timing statistics for device
            if (device->ikt_count > 0) {
                struct timing_stats stats = detector_get_stats(device);
                printf("────────────────────────────────────────────────────────────────\n");
                printf("  Timing: Mean IKT: %7.2f ms | Jitter: %7.2f ms | Samples: %d\n",
                       stats.mean_ikt, stats.jitter, stats.sample_count);
                printf("────────────────────────────────────────────────────────────────\n\n");
            }
        }
        state->cmd_length = 0;
        state->cmd_buffer[0] = '\0';
        state->backspace_count = 0;  // Reset for next command
    } else if (ev->code == KEY_BACKSPACE) {
        // Remove last character and track mistake (human indicator)
        state->backspace_count++;
        if (state->cmd_length > 0) {
            state->cmd_length--;
            state->cmd_buffer[state->cmd_length] = '\0';
        }
    } else {
        // Try to add printable character
        char c = input_keycode_to_char(ev->code, shift_active);
        if (c != '\0' && state->cmd_length < CMD_BUFFER_SIZE - 1) {
            state->cmd_buffer[state->cmd_length++] = c;
            state->cmd_buffer[state->cmd_length] = '\0';
        }
    }

    return ikt_ms;
}

struct timing_stats detector_get_stats(struct device_info *device) {
    struct timing_stats stats;
    stats.sample_count = device->ikt_count;
    stats.total_keys = device->total_keys;
    stats.mean_ikt = calculate_mean(device->ikt_buffer, device->ikt_count);
    stats.jitter = calculate_jitter(device->ikt_buffer, device->ikt_count, stats.mean_ikt);
    return stats;
}

// ===== PATTERN MATCHING =====

// Substring search
static int contains_pattern(const char *str, const char *pattern) {
    const char *search_pos = str;
    while (*search_pos) {
        const char *str_ptr = search_pos;
        const char *pat_ptr = pattern;
        while (*str_ptr && *pat_ptr &&
               (*str_ptr == *pat_ptr ||
                (*str_ptr >= 'A' && *str_ptr <= 'Z' && *str_ptr + 32 == *pat_ptr) ||
                (*pat_ptr >= 'A' && *pat_ptr <= 'Z' && *pat_ptr + 32 == *str_ptr))) {
            str_ptr++;
            pat_ptr++;
        }
        if (!*pat_ptr) return 1;
        search_pos++;
    }
    return 0;
}

// Count occurrences of a character
static int count_char(const char *str, char c) {
    int count = 0;
    while (*str) {
        if (*str == c) count++;
        str++;
    }
    return count;
}

// (curl, wget)
static int detect_download_commands(const char *cmd) {
    return contains_pattern(cmd, "curl ") ||
           contains_pattern(cmd, "wget ") ||
           contains_pattern(cmd, "curl\"") ||
           contains_pattern(cmd, "wget\"");
}

// (curl | sh, wget | bash)
static int detect_piped_download(const char *cmd) {
    if (!detect_download_commands(cmd)) return 0;

    // Check for pipe to shell
    return (contains_pattern(cmd, "| sh") ||
            contains_pattern(cmd, "| bash") ||
            contains_pattern(cmd, "|sh") ||
            contains_pattern(cmd, "|bash"));
}

static int detect_base64(const char *cmd) {
    int has_base64_decode = contains_pattern(cmd, "base64") &&
                           (contains_pattern(cmd, "-d") || contains_pattern(cmd, "--decode"));

    int piped_to_shell = contains_pattern(cmd, "| sh") ||
                        contains_pattern(cmd, "| bash") ||
                        contains_pattern(cmd, "|sh") ||
                        contains_pattern(cmd, "|bash");

    return has_base64_decode && piped_to_shell;
}

// eval with command substitution
static int detect_eval(const char *cmd) {
    return contains_pattern(cmd, "eval") &&
           (contains_pattern(cmd, "$(") || contains_pattern(cmd, "`"));
}

static int detect_reverse_shell(const char *cmd) {
    return contains_pattern(cmd, "/dev/tcp/") ||
           contains_pattern(cmd, "bash -i") ||
           contains_pattern(cmd, "sh -i") ||
           contains_pattern(cmd, "nc -e") ||
           contains_pattern(cmd, "ncat -e");
}

static int detect_execution_patterns(const char *cmd) {
    int has_chmod = contains_pattern(cmd, "chmod") && contains_pattern(cmd, "+x");
    int has_execute = contains_pattern(cmd, "./");

    return has_chmod || has_execute;
}

static int detect_persistence(const char *cmd) {
    // crontab modifications
    int has_crontab = contains_pattern(cmd, "crontab");

    // bashrc/profile modifications
    int has_rc_edit = (contains_pattern(cmd, ".bashrc") ||
                       contains_pattern(cmd, ".profile") ||
                       contains_pattern(cmd, ".bash_profile")) &&
                      (contains_pattern(cmd, "echo") ||
                       contains_pattern(cmd, ">>") ||
                       contains_pattern(cmd, "sed") ||
                       contains_pattern(cmd, "vi") ||
                       contains_pattern(cmd, "nano"));

    return has_crontab || has_rc_edit;
}

static int detect_command_chaining(const char *cmd) {
    int pipe_count = count_char(cmd, '|');
    return pipe_count > 3;
}

void detector_analyze_command(struct detector_state *state, struct device_info *device, const char *command) {
    if (!command || command[0] == '\0') return;

    // Reset pattern flags
    state->pattern_download = 0;
    state->pattern_piped_download = 0;
    state->pattern_base64 = 0;
    state->pattern_eval = 0;
    state->pattern_reverse_shell = 0;
    state->pattern_execution = 0;
    state->pattern_persistence = 0;
    state->pattern_command_chain = 0;

    // Detect command patterns
    state->pattern_download = detect_download_commands(command);
    state->pattern_piped_download = detect_piped_download(command);
    state->pattern_base64 = detect_base64(command);
    state->pattern_eval = detect_eval(command);
    state->pattern_reverse_shell = detect_reverse_shell(command);
    state->pattern_execution = detect_execution_patterns(command);
    state->pattern_persistence = detect_persistence(command);
    state->pattern_command_chain = detect_command_chaining(command);

    // Reverse shells are malicious
    if (state->pattern_reverse_shell) {
        state->current_score = 150;
        state->pattern_fast_timing = 2;  // Display automation warning
        goto display_alert;
    }

    // Calculate base severity (0-10 scale based on pattern maliciousness)
    double base_severity = 0.0;

    // Highest severity pattern wins
    if (state->pattern_eval) base_severity = 8.0; // High risk
    else if (state->pattern_persistence) base_severity = 7.0; // High risk
    else if (state->pattern_base64) base_severity = 6.5; // Moderate-High
    else if (state->pattern_piped_download) base_severity = 5.0; // Moderate
    else if (state->pattern_execution) base_severity = 3.0; // Low-Moderate
    else if (state->pattern_command_chain) base_severity = 2.5; // Low
    else if (state->pattern_download) base_severity = 2.0; // Low

    // Calculate timing confidence multiplier (1.0 - 4.0)
    double timing_multiplier = 1.0;
    state->pattern_fast_timing = 0;

    if (device->ikt_count >= 10) {
        struct timing_stats stats = detector_get_stats(device);

        // Very fast typing: ~95% confidence it's automated
        if (stats.mean_ikt < 5.0 && stats.jitter < 10.0) {
            timing_multiplier = 3.8;
            state->pattern_fast_timing = 2;
        }
        // Fast typing: ~80% confidence it's automated
        else if (stats.mean_ikt < 15.0 && stats.jitter < 100.0) {
            timing_multiplier = 2.6;
            state->pattern_fast_timing = 1;
        }
        // Moderately fast: ~30% confidence (could be fast human)
        else if (stats.mean_ikt < 50.0) {
            timing_multiplier = 1.5;
        }
        // Normal human timing: ~5% baseline suspicion
        else {
            timing_multiplier = 1.05;
        }
    }

    // Human behavior indicators (mistakes = human)
    if (state->backspace_count > 0 && state->pattern_fast_timing > 0) {
        // Reduce timing multiplier by 30-50% based on mistake frequency
        double human_factor = 1.0 - (0.3 + (state->backspace_count * 0.05));
        if (human_factor < 0.5) human_factor = 0.5;  // Max 50% reduction
        timing_multiplier *= human_factor;

        // Downgrade automation flag if mistakes were made
        if (state->pattern_fast_timing == 2 && state->backspace_count >= 2) {
            state->pattern_fast_timing = 1;  // Demote to "likely" from "certain"
        } else if (state->pattern_fast_timing == 1 && state->backspace_count >= 3) {
            state->pattern_fast_timing = 0;  // Clear automation flag
        }
    }

    // Calculate final score
    double final_score = base_severity * timing_multiplier * 10.0;

    // FHigh-severity patterns can't fall too low
    if (base_severity >= 7.0 && final_score < 60.0) {
        final_score = 60.0;
    }

    state->current_score = (int)final_score;

display_alert:

    // Display pattern analysis if patterns detected
    if (state->current_score > 0) {
        printf("┌─────────────────────────────────────────────────────────────────┐\n");
        printf("│ THREAT DETECTION ALERT                                          │\n");
        printf("├─────────────────────────────────────────────────────────────────┤\n");

        // Command-based patterns
        if (state->pattern_download) {
            printf("│ Download command (curl/wget)                            │\n");
        }
        if (state->pattern_piped_download) {
            printf("│ Download piped to shell (may be legitimate installer)   │\n");
        }
        if (state->pattern_base64) {
            printf("│ Base64 decode obfuscation                               │\n");
        }
        if (state->pattern_eval) {
            printf("│ Eval with command substitution                          │\n");
        }
        if (state->pattern_reverse_shell) {
            printf("│ REVERSE SHELL - Active exploitation!                 │\n");
        }
        if (state->pattern_execution) {
            printf("│ Execution pattern (chmod +x / ./)                       │\n");
        }
        if (state->pattern_persistence) {
            printf("│ Persistence mechanism (crontab/bashrc)                  │\n");
        }
        if (state->pattern_command_chain) {
            printf("│ Command chaining detected                               │\n");
        }

        // Timing-based patterns
        if (state->pattern_fast_timing == 2) {
            printf("│ AUTOMATION: Very fast typing detected (<5ms IKT)        │\n");
        } else if (state->pattern_fast_timing == 1) {
            printf("│ AUTOMATION: Fast typing detected (<15ms IKT)            │\n");
        }

        // Human behavior indicators (only display if score is reduced)
        if (state->backspace_count > 0 && state->pattern_fast_timing > 0) {
            printf("│ HUMAN INDICATOR: %d backspace(s) detected (reduced score) │\n", state->backspace_count);
        }

        printf("├─────────────────────────────────────────────────────────────────┤\n");
        printf("│ TOTAL THREAT SCORE: %-3d                                        │\n", state->current_score);

        if (state->current_score >= 100) {
            printf("│ THREAT LEVEL: CRITICAL - Active exploitation              │\n");
        } else if (state->current_score >= 60) {
            printf("│ THREAT LEVEL: HIGH - Likely malicious activity            │\n");
        } else if (state->current_score >= 30) {
            printf("│ THREAT LEVEL: MEDIUM - Suspicious, investigate            │\n");
        } else {
            printf("│ THREAT LEVEL: LOW - Monitor only                          │\n");
        }

        printf("└─────────────────────────────────────────────────────────────────┘\n\n");
    }
}
