# USB HID Keystroke Injection: Attack and Defense

A Linux security project demonstrating USB HID keystroke injection attacks and OS-level defense mechanisms through kernel module development and user-space monitoring.

## Project Overview

This project explores USB Human Interface Device (HID) security from both offensive and defensive perspectives, implementing:

1. **USB HID Attack Vector** (Arduino): Firmware that enumerates as a USB keyboard and executes pre-programmed keystroke sequences
2. **Kernel-Level Monitoring** (Linux Kernel Module): Monitors USB bus activity and detects HID device connections
3. **User-Space Detection** (Daemon): Analyzes keystroke patterns to identify injection attacks using timing analysis and behavioral signatures

## Project Structure

```
HID-Injection-Guard/
├── README.md                           # Main project documentation (this file)
│
├── arduino/                            # Arduino HID injection firmware
│   ├── payloads/                       # Attack payload implementations
│   │   ├── README.md                   # Payload documentation
│   │   ├── revshell.ino                # Reverse shell with evidence hiding
│   │   ├── simple_payload.ino          # Basic keystroke injection
│   │   └── terminal_payload.ino        # Terminal command execution
│   └── README.md                       # Arduino component documentation
│
├── daemon/                             # User-space input monitoring daemon
│   ├── detector.c                      # Timing analysis and pattern matching
│   ├── detector.h                      # Detector interface
│   ├── hid_guard.c                     # Main daemon orchestration
│   ├── input_handler.c                 # Device discovery and event reading
│   ├── input_handler.h                 # Input handler interface
│   ├── Makefile                        # Daemon build configuration
│   └── README.md                       # Daemon architecture documentation
│
├── docs/                               # Project documentation and reports
│   ├── PROGRESS_REPORT.md              # Implementation progress and challenges
│   ├── README.md                       # Documentation index
│   ├── TECHNICAL_REPORT.md             # Technical report ⭐
│   └── USB_HID_Security_Project.md     # Project specification
│
├── kernel/                             # Linux kernel module for USB monitoring
│   ├── Makefile                        # Kernel module build configuration
│   ├── README.md                       # Kernel module documentation
│   └── usb_monitor.c                   # Kernel module source code
│
└── tests/                              # Testing framework and validation
    ├── Makefile                        # Test build configuration
    ├── README.md                       # Testing framework documentation
    └── test_detector.c                 # Unit tests for detector logic
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

The daemon monitors input devices and performs multi-layered analysis:

**Device Identification:**
- **USB Device Recognition**: Integrates with kernel module to identify devices (VID/PID/manufacturer/product)
- **Dynamic Device Detection**: Automatically monitors new input devices as they connect via inotify
- **Device Tracking**: Associates USB metadata with input events for forensic analysis

**Timing Analysis:**
- **Inter-Keystroke Timing (IKT)**: Measures time between consecutive keystrokes (1-4ms = injection, 50-200ms = human)
- **Jitter Analysis**: Calculates standard deviation of keystroke timing (detects unnaturally consistent patterns)
- **Per-Device Statistics**: Tracks timing metrics, keystroke counts, and connection duration

**Behavioral Analysis:**
- **Command Reconstruction**: Buffers keystrokes and reconstructs complete commands with shift/caps lock tracking
- **Character Mapping**: Converts keycodes to printable characters for pattern analysis

**Detection Signature:**
HID injection shows consistent IKT ~1-4ms (physically impossible for humans), while human typing typically ranges from 50-200ms depending on typing speed.

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

### Project Documentation
- **Project Specification**: `docs/USB_HID_Security_Project.md`
- **Technical Report**: `docs/TECHNICAL_REPORT.md`
- **Progress Report**: `docs/PROGRESS_REPORT.md`
- **Component READMEs**: See README.md in each subdirectory for detailed component documentation

### Quick Links
- [Kernel Module Documentation](kernel/README.md)
- [Daemon Documentation](daemon/README.md)
- [Arduino Payloads Documentation](arduino/README.md)
- [Testing Documentation](tests/README.md)

## Academic Context

This project demonstrates core OS concepts including:

- **Kernel Module Development**: Loadable kernel modules, USB notifier chains, reference counting
- **Device Driver Interaction**: USB subsystem architecture, HID class drivers, input event layer
- **Process Management**: User-space daemons, resource management, privilege handling
- **USB Subsystem Architecture**: Device enumeration, descriptor parsing, sysfs interface
- **System Security Mechanisms**: Behavioral analysis, anomaly detection, defense in depth
- **User/Kernel Space Communication**: Sysfs virtual filesystem for data exchange