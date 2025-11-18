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
│   │   ├── simple_payload.ino
│   │   └── terminal_payload.ino
│   └── README.md
├── kernel/              # Linux kernel module for USB monitoring
│   ├── Makefile         # Kernel module build configuration
│   ├── usb_monitor.c    # Kernel module source code
│   └── README.md
├── daemon/              # User-space input monitoring daemon
│   └── README.md
├── tests/               # Testing scripts and validation
│   ├── scripts/         # Test automation scripts
│   ├── input-samples/   # Recorded keystroke patterns
│   └── README.md
├── docs/                # Project documentation and reports
│   ├── reports/         # Technical reports
│   └── README.md
└── README.md            # This file
```

## Quick Start

### 1. Build and Load Kernel Module
```bash
cd kernel
make
sudo insmod usb_monitor.ko
sudo dmesg | tail -n 20  # Verify loading

# Connect a USB HID device

# View HID device information via sysfs
cat /sys/kernel/usb_hid_monitor/vid
cat /sys/kernel/usb_hid_monitor/pid
cat /sys/kernel/usb_hid_monitor/manufacturer
cat /sys/kernel/usb_hid_monitor/product

# View all attributes at once
grep -H . /sys/kernel/usb_hid_monitor/*
```

### 2. Build and Run Daemon
```bash
cd daemon
make
sudo ./hid_guard
```

### 3. Compile and Upload Arduino Firmware
```bash
cd arduino
arduino-cli compile --fqbn arduino:avr:micro hid_injector.ino
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:micro hid_injector.ino
```

## Detection Features

The daemon monitors input devices and performs timing analysis:

- **Inter-Keystroke Timing (IKT)**: Measures time between consecutive keystrokes
- **Jitter Analysis**: Calculates standard deviation of keystroke timing (detects unnaturally consistent patterns)
- **Dynamic Device Detection**: Automatically monitors new input devices as they connect
- **Per-Device Statistics**: Tracks timing metrics, keystroke counts, and connection duration

Detection signature: HID injection shows consistent IKT ~1-4ms (physically impossible for humans), while human typing typically ranges from 50-200ms depending on typing speed.

## Requirements

- Linux system with kernel headers
- GCC compiler with kernel module support
- Arduino Micro
- Arduino IDE or arduino-cli
- Root/sudo privileges for kernel operations
- libevdev development headers (optional)

## Testing

Test the system by connecting the Arduino device with the kernel module and daemon running. Monitor kernel logs and daemon output to observe detection behavior.

## Documentation

- Project specification: `docs/USB_HID_Security_Project.pdf`
- Progress report: `docs/PROGRESS_REPORT.md`
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