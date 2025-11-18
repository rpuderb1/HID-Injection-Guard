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

- Registers USB notifiers to detect device connection/disconnection events
- Identifies HID class devices (interface class 0x03)
- Extracts device information (VID, PID, manufacturer, product, serial)
- Exposes device information via sysfs at `/sys/kernel/usb_hid_monitor/`
- Logs device connection/disconnection events to kernel log

## Sysfs Interface

The module creates the following sysfs attributes:

- `/sys/kernel/usb_hid_monitor/vid` - Vendor ID (hexadecimal)
- `/sys/kernel/usb_hid_monitor/pid` - Product ID (hexadecimal)
- `/sys/kernel/usb_hid_monitor/manufacturer` - Manufacturer string
- `/sys/kernel/usb_hid_monitor/product` - Product name string
- `/sys/kernel/usb_hid_monitor/serial` - Serial number string

These attributes contain information about the most recently connected HID device.

## Requirements

- Linux kernel headers matching your running kernel
- Root/sudo privileges for module operations