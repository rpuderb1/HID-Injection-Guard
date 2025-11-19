#include "detector.h"
#include "input_handler.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <linux/input-event-codes.h>

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

double detector_process_keystroke(struct detector_state *state, struct input_event *ev) {
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
    if (state->has_last_key) {
        ikt_ms = timeval_diff_ms(&state->last_key_time, &ev->time);

        // Store in circular buffer
        state->ikt_buffer[state->ikt_index] = ikt_ms;
        state->ikt_index = (state->ikt_index + 1) % IKT_BUFFER_SIZE;
        if (state->ikt_count < IKT_BUFFER_SIZE) {
            state->ikt_count++;
        }
    }

    state->last_key_time = ev->time;
    state->has_last_key = 1;
    state->total_keys++;

    // Handle command buffering
    int shift_active = state->shift_pressed || state->caps_lock_active;

    if (ev->code == KEY_ENTER) {
        // Command completed
        if (state->cmd_length > 0) {
            printf("\n"
                   "╔════════════════════════════════════════════════════════════════\n"
                   "║ COMMAND: %s\n"
                   "╚════════════════════════════════════════════════════════════════\n\n",
                   state->cmd_buffer);
        }
        state->cmd_length = 0;
        state->cmd_buffer[0] = '\0';
    } else if (ev->code == KEY_BACKSPACE) {
        // Remove last character
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

struct timing_stats detector_get_stats(struct detector_state *state) {
    struct timing_stats stats;
    stats.sample_count = state->ikt_count;
    stats.total_keys = state->total_keys;
    stats.mean_ikt = calculate_mean(state->ikt_buffer, state->ikt_count);
    stats.jitter = calculate_jitter(state->ikt_buffer, state->ikt_count, stats.mean_ikt);
    return stats;
}

int detector_should_show_stats(struct detector_state *state) {
    return (state->total_keys % STATS_INTERVAL == 0 && state->ikt_count > 0);
}

void detector_display_stats(struct detector_state *state) {
    struct timing_stats stats = detector_get_stats(state);

    printf("\n");
    printf("========== TIMING STATISTICS (last %d keystrokes) ==========\n", stats.sample_count);
    printf("  Mean IKT:    %7.2f ms\n", stats.mean_ikt);
    printf("  Jitter (SD): %7.2f ms\n", stats.jitter);
    printf("  Total keys:  %d\n", stats.total_keys);
    printf("===========================================================\n\n");
}

const char* detector_get_completed_command(struct detector_state *state) {
    // TODO: Return command for pattern matching
    return state->cmd_buffer;
}
