# Linux Kernel Module - USB Monitor

This directory contains the Linux kernel module (LKM) for monitoring USB HID device connections.

## Files

- **usb_monitor.c**: Main kernel module source code
- **Makefile**: Build configuration for the kernel module

## Prerequisites

Kernel Headers:

```bash
sudo apt install linux-headers-$(uname -r)
```

## Building

```bash
make
```

This compiles the kernel module using the current kernel headers.

## Loading and Testing

```bash
# Load module
sudo insmod usb_monitor.ko

# View kernel logs
sudo dmesg | tail -n 50

# Check loaded modules
lsmod | grep usb_monitor

# Unload the module
sudo rmmod usb_monitor

# Clean build artifacts
make clean
```

## Key Functionality

- Register USB notifiers to detect device events
- Identify HID class devices (interface class 0x03)
- Extract device information (VID, PID, manufacturer, serial)
- Create /sys entries for userspace communication
- Log device connection/disconnection events

## Requirements

- Linux kernel headers matching your running kernel
- Root/sudo privileges for module operations