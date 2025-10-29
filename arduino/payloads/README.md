# Attack Payloads

This directory contains different keystroke injection payload implementations for testing.

## Payload Examples

Create separate `.ino` files for different attack scenarios:

- **simple_payload.ino**: Basic keystroke injection test
- **terminal_payload.ino**: Opens terminal and executes commands
- **stealth_payload.ino**: Uses delays to evade timing-based detection
- **rapid_payload.ino**: Maximum speed injection for detection testing
- **complex_payload.ino**: Attack with modifier keys

## Usage

Each payload should be a self-contained Arduino sketch that can be uploaded independently to test different attack patterns and detection capabilities.