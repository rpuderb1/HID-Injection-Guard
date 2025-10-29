# Arduino HID Injection Firmware

This directory contains Arduino firmware for USB HID keystroke injection attacks.

## Structure

- **payloads/**: Different attack payload implementations

## Development

### Compile and Upload
```bash
arduino-cli compile --fqbn arduino:avr:micro hid_injector.ino
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:micro hid_injector.ino
```

### Monitor Serial Output
```bash
arduino-cli monitor -p /dev/ttyACM0
```

## Board Configuration

- **Target Board**: Arduino Micro
- **FQBN**: `arduino:avr:micro`

## Notes

The Arduino Keyboard library allows the device to enumerate as a USB keyboard. This demonstrates the fundamental USB HID security vulnerability.