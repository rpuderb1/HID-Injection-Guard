#include <stdio.h>
#include <string.h>
#include "input_handler.h"
#include "detector.h"

/**
 * HID Guard - USB Keystroke Injection Detector
 *
 * Architecture:
 *   input_handler.c - Device discovery, event reading, keycode parsing
 *   detector.c      - Timing analysis, command buffering, attack detection
 *   hid_guard.c     - Main loop orchestration (this file)
 *
 */

int main(void) {
    struct input_state input;
    struct detector_state detector;
    struct input_event ev;

    if (input_init(&input) < 0) {
        fprintf(stderr, "Failed to initialize input monitoring\n");
        return 1;
    }

    detector_init(&detector);

    // Main event loop
    while (1) {
        // Wait for input events
        if (input_poll(&input) < 0) {
            break;
        }

        // Handle new device connections
        if (input_has_new_devices(&input)) {
            input_process_new_devices(&input);
        }

        // Process events from all monitored devices
        for (int device_idx = 0; device_idx < input.num_devices; device_idx++) {
            int ret = input_read_event(&input, device_idx, &ev);

            if (ret < 0) {
                // Device disconnected - array shifted, recheck this index
                device_idx--;
                continue;
            }

            if (ret == 0) {
                // No event available
                continue;
            }

            // Get device info for device tracking
            struct device_info *dev = input_get_device_info(&input, device_idx);

            // Filter to key presses only
            if (!input_is_key_press(&ev)) {
                // Still process for shift tracking
                detector_process_keystroke(&detector, dev, &ev);
                continue;
            }

            // Analyze keystroke timing and buffer command
            double ikt_ms = detector_process_keystroke(&detector, dev, &ev);

            // Update device keystroke counter
            dev->keystroke_count++;

            // Display keystroke with timing
            const char* key_name = input_keycode_name(ev.code);

            // Show device identification
            if (strcmp(dev->product, "Unknown") != 0) {
                printf("[%s] ", dev->product);
            } else {
                printf("[%s] ", dev->path);
            }

            if (strcmp(key_name, "UNKNOWN") == 0) {
                printf("Key: UNKNOWN (code=%d)", ev.code);
            } else {
                printf("Key: %-12s", key_name);
            }

            if (ikt_ms > 0) {
                printf(" | IKT: %7.2f ms", ikt_ms);
            }
            printf("\n");
        }
    }

    // Cleanup
    input_cleanup(&input);
    return 0;
}
