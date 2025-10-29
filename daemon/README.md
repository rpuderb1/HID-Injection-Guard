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

## Detection Algorithm

The daemon implements a weighted scoring system:

- **+25 points**: Low IKT (fast typing) + Low Jitter (consistent timing)
- **+75 points**: Rapid system commands (payload execution patterns)
- **-100 points**: Human signature detected (backspace or pause >1 second)
- **Threshold**: Score ≥ 100 triggers detection alert

## Key Functionality

- Monitor /dev/input/eventX devices for keyboard events
- Calculate Inter-Keystroke Timing (IKT) and jitter
- Detect suspicious keystroke patterns
- Log alerts to syslog or console

## Requirements

- Root/sudo privileges for /dev/input access
- libevdev development headers (optional, for enhanced input handling)