# USB HID Keystroke Injection: Attack and Defense

A Linux security project demonstrating USB HID keystroke injection attacks and OS-level defense mechanisms through kernel module development and user-space monitoring.

## Project Overview

This project explores USB Human Interface Device (HID) security from both offensive and defensive perspectives, implementing:

1. **USB HID Attack Vector** (Arduino): Firmware that enumerates as a USB keyboard and executes pre-programmed keystroke sequences
2. **Kernel-Level Monitoring** (Linux Kernel Module): Monitors USB bus activity and detects HID device connections
3. **User-Space Detection** (Daemon): Analyzes keystroke patterns to identify injection attacks using timing analysis and behavioral signatures

## Project Structure

```
├── arduino/              # Arduino HID injection firmware
│   ├── payloads/        # Different attack payload implementations
│   └── tests/           # Arduino functionality tests
├── kernel/              # Linux kernel module for USB monitoring
├── daemon/              # User-space input monitoring daemon
├── tests/               # Testing scripts and validation
│   ├── scripts/         # Test automation scripts
│   └── input-samples/   # Recorded keystroke patterns
├── docs/                # Project documentation and reports
│   └── reports/         # Technical reports
└── README.md            # This file
```

## Quick Start

### 1. Build and Load Kernel Module
```bash
cd kernel
make
sudo insmod usb_monitor.ko
sudo dmesg | tail -n 20  # Verify loading
```

### 2. Build and Run Daemon
```bash
cd daemon
make
sudo ./hid_guard --debug
```

### 3. Compile and Upload Arduino Firmware
```bash
cd arduino
arduino-cli compile --fqbn arduino:avr:micro hid_injector.ino
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:micro hid_injector.ino
```

## Detection Algorithm

The system uses a weighted scoring system to detect keystroke injection:

| Observation | Signature | Score |
|-------------|-----------|-------|
| **Keystroke Timing** | Low IKT + Low Jitter (very fast, consistent) | +25 |
| **Behavior/Intent** | Rapid system commands (payload execution) | +75 |
| **Human Signature** | Backspace or pause >1 second | -100 |

**Detection Threshold**: Score ≥ 100 indicates potential injection attack

## Requirements

- Linux system with kernel headers
- GCC compiler with kernel module support
- Arduino Micro
- Arduino IDE or arduino-cli
- Root/sudo privileges for kernel operations
- libevdev development headers (optional)

## Testing

Run the test suite to validate detection accuracy:
```bash
cd tests/scripts
./load_system.sh
./run_tests.sh
./analyze_logs.sh
```

## Documentation

- Project specification: `docs/USB_HID_Security_Project.pdf`
- Technical reports: `docs/reports/`
- Per-directory documentation: See README.md in each subdirectory

## Academic Context

This project demonstrated core OS concepts including:
- Kernel module development
- Device driver interaction
- Process management
- USB subsystem architecture
- System security mechanisms
- User/kernel space communication