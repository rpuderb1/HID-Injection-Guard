# User-Space Input Monitoring Daemon

This directory contains the user-space daemon that monitors input events and detects keystroke injection attacks.

## Files

- **hid_guard.c**: Main daemon source code
- **Makefile**: Build configuration (optional)

## Building

### Using make
```bash
make
```

### gcc
```bash
gcc -o hid_guard hid_guard.c -lpthread
```

### Debug build
```bash
make debug
```

## Running

```bash
# Run with root privileges (required for /dev/input access)
sudo ./hid_guard

# Run in debug mode
sudo ./hid_guard --debug

# Run as background daemon
sudo ./hid_guard --daemon
```

## Implemented Features

- **Input Event Monitoring**: Reads raw input events from `/dev/input/eventX` devices
- **Keystroke Parsing**: Filters EV_KEY events and maps keycodes to key names
- **Dynamic Device Detection**: Uses inotify to automatically detect and monitor new input devices as they connect
- **Timing Analysis**:
  - Calculates Inter-Keystroke Timing (IKT) in milliseconds
  - Tracks IKT values in circular buffer (last 100 keystrokes)
  - Calculates mean IKT and jitter (standard deviation)
  - Displays timing statistics every 10 keystrokes
- **Device Management**:
  - Handles device disconnections gracefully with consecutive error counter
  - Tracks per-device statistics (connection time, keystroke count)
  - Logs device lifecycle events

## Detection Signature

HID injection attacks show distinctive timing patterns:
- **Injection**: IKT ~1-4ms (extremely fast, consistent keystroke timing)
- **Human typing**: IKT typically 50-200ms (varies by typing speed)

## Requirements

- Root/sudo privileges for /dev/input access
- libevdev development headers (optional, for enhanced input handling)