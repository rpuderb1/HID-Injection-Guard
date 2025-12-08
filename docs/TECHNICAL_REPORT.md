# USB HID Keystroke Injection: Attack Vector and OS-Level Defense Mechanisms

**COSC 439: Operating Systems**

---

## 1. Introduction

### Purpose and Overview

This project demonstrates operating system security concepts by implementing a USB HID keystroke injection attack and OS-level defense mechanisms. The system explores vulnerabilities at the hardware-software interface utilizing kernel module development, device driver interaction, process management, and behavioral security analysis.

### Project Components

1. **Arduino HID Attack Vector** - Firmware that enumerates as a USB keyboard and executes keystroke injection payloads
2. **Linux Kernel Module** - Monitors USB bus activity and exposes HID device metadata via sysfs
3. **User-Space Daemon** - Analyzes keystroke timing and patterns to detect injection attacks in real-time

## 2. Attack Implementation

### Arduino USB HID Enumeration

The attack uses an **Arduino Micro** (ATmega32U4). The Arduino Keyboard library leverages the USB HID class specification (class 0x03) to enumerate as a standard keyboard. Linux automatically loads the `usbhid` module and creates `/dev/input/eventX` device files.

### Attack Payloads

Payloads:

**1. Simple Payload** (`simple_payload.ino`)
- Basic text injection demonstrating fast keystroke timing (1-4ms inter-keystroke timing)
- Used for initial detection testing

**2. Terminal Payload** (`terminal_payload.ino`)
- Opens terminal using keyboard shortcut (Super+Enter)
- Executes: `htop`
- Demonstrates command injection

**3. Reverse Shell Payload** (`revshell.ino`)
- Establishes malicious executable 
- Creates a reverse connection to attackers listener
- Clears command history to hide evidence
- Demonstrates stealth techniques

### Attack Characteristics

- **Inter-Keystroke Timing (IKT)**: 1-4ms (vs. 50-200ms for human typing)
- **Jitter**: <1ms (vs. 15-30ms for humans)
- **Execution Window**: 2-4 seconds after USB connection
- **No Authentication**: OS trusts USB keyboards implicitly

---

## 3. Defense Mechanisms

### Kernel Module Architecture

**File**: `kernel/usb_monitor.c`

The kernel module monitors USB device connections and exposes metadata to user space:

**Key Functions:**
- **USB Notifier Registration**: `usb_register_notify()` intercepts USB device events
- **HID Detection**: Iterates through device interfaces to detect HID class (0x03)
- **Metadata Extraction**: Extracts VID/PID/manufacturer/product/serial from USB descriptors
- **Sysfs Interface**: Exposes device information at `/sys/kernel/usb_hid_monitor/`

**Sysfs Attributes:**
```
/sys/kernel/usb_hid_monitor/vid
/sys/kernel/usb_hid_monitor/pid
/sys/kernel/usb_hid_monitor/manufacturer
/sys/kernel/usb_hid_monitor/product
/sys/kernel/usb_hid_monitor/serial
```

**Limitation**: Only tracks the last connected HID device.

### User-Space Daemon Architecture

The daemon uses a three-module design:

#### Module 1: Input Handler (`input_handler.c/h`)

Handles device discovery and event management:

- **Device Discovery**: Scans `/dev/input/event*` devices at startup
- **Hotplug Detection**: Uses `inotify` to detect new device files immediately (<10ms latency) [Prevent Race Condition]
- **Event Multiplexing**: Uses `poll()` to monitor multiple devices simultaneously
- **USB Metadata**: Reads device information from kernel module sysfs interface
- **Per-Device Tracking**: Maintains timing buffers (100 IKT values per device)
- **Disconnect Handling**: Uses consecutive error counting (threshold: 10) to detect disconnections

#### Module 2: Detector (`detector.c/h`)

Performs timing analysis and pattern matching:

**Timing Analysis:**
- Calculates Inter-Keystroke Timing (IKT) in milliseconds
- Computes mean and jitter (standard deviation)
- Detection threshold: Mean IKT <5ms with jitter <2ms indicates injection

**Command Reconstruction:**
- Buffers keystrokes into command strings
- Handles shift and caps lock state
- Converts keycodes to printable characters

**Pattern Matching:**
Detects malicious command patterns:
- Download commands: `wget`, `curl`
- Reverse shells: `/dev/tcp`, `nc -e`, `bash -i`
- Base64 encoding: `base64`, `b64`
- Command chaining: `&&`, `;`, `|`
- Persistence: `crontab`, `systemctl`, `.bashrc`

**Threat Scoring:**
```
Base Severity (0-10 scale):
  - Reverse shell patterns: 10
  - Persistence mechanisms: 8
  - Download commands: 6
  - Base64 encoding: 4
  - Command chaining: 3

Timing Multiplier:
  - IKT < 2ms: 3.0x (extremely fast)
  - IKT < 4ms: 2.0x (very fast)
  - IKT < 6ms: 1.5x (moderately fast)
  - IKT >= 6ms: 1.0x (normal)

Final Score = Base Severity × Timing Multiplier × 10

Thresholds:
  - Score >= 100: CRITICAL ALERT
  - Score >= 60: WARNING
  - Score >= 30: SUSPICIOUS
```

**Human Behavior Detection:**
- Backspace key detection (reduces score)
- Pauses > 1 second between keystrokes (reduces score)

#### Module 3: Main Orchestration (`hid_guard.c`)

Coordinates the system:
- Initializes input handler and detector
- Main event loop polls for input events
- Checks for new device connections
- Routes events to detector for analysis

### Race Condition Solution

**Problem**: Arduino payloads execute 2 seconds after power-on, but device file appears at ~1 second.

**Solution**:
- `inotify` detects device file creation within 2-10ms
- Device opened immediately with zero delay
- Provides ~980ms buffer before payload execution
- Time to start monitoring before first keystroke

---

## 4. OS Concepts Analysis

### Device Driver Concepts

**USB and Input Subsystem Layers:**
```
User Space:      /dev/input/eventX
                      ↓
Kernel Space:    Input Subsystem
                      ↓
                 usbhid Driver
                      ↓
                 USB Core
                      ↓
Hardware:        USB Device
```

---

## 5. Testing and Results

### Test Environment

- **Host**: Linux kernel 6.17.4 (PopOS 24.04 LTS)
- **Attack Device**: Arduino Micro (ATmega32U4)

### Detection Effectiveness

| Test Scenario | Detection | False Positive | IKT (ms) |
|--------------|-----------|----------------|----------|
| Simple Payload | ✅ Detected | ❌ No | 1.8 |
| Terminal Command | ✅ Detected | ❌ No | 2.3 |
| Reverse Shell | ✅ Detected | ❌ No | 1.9 |
| Fast Typing (120 WPM) | ❌ Not detected | ❌ No | 83 |
| Very Fast Typing (150 WPM) | ❌ Not detected | ❌ No | 67 |

**Results:**
- **True Positive Rate**: 100% (3/3 attacks detected)
- **False Positive Rate**: 0% (0/3 human scenarios)
- **Detection Latency**: <100ms from first keystroke

### Timing Analysis Comparison

```
Human Typing (100 WPM):
  Mean IKT: 92ms
  Jitter:   24ms
  Range:    45-180ms

HID Injection (Arduino):
  Mean IKT: 2.1ms
  Jitter:   0.4ms
  Range:    1.8-2.5ms
```

### Performance Impact

- **File Descriptors**: ~7 (devices + inotify + stdio)
- **Kernel Module**: negligible CPU impact
- **Latency**: No perceptible impact on system responsiveness

---

## 6. Security Analysis

### Defense Effectiveness

**Strengths:**
- Timing-based detection is highly effective (100% detection in testing)
- Pattern matching identifies common attack commands
- Real-time monitoring with low latency (<100ms)
- Minimal performance impact
- No false positives with human typing

**Weaknesses:**
1. **Evasion Potential**: Attackers can add delays between keystrokes to mimic human timing
2. **Single-Device Limitation**: Kernel module only tracks last HID device
3. **Post-Execution Detection**: Detects during attack (no preventative blocking yet)
4. **Pattern Evasion**: Attackers can obfuscate commands (string concatenation)

### Evasion Techniques

Attackers could potentially evade detection through:

1. **Slow Injection**: Add delays between keystrokes (60-120ms) to mimic human timing
2. **Jitter Injection**: Add random variance to timing
3. **Human Behavior Simulation**: Include random backspaces and pauses
4. **Command Obfuscation**: Use string concatenation or variable expansion

### Mitigation Recommendations

**System-Level Defenses:**
- USB whitelisting
- Block HID enumeration on untrusted devices
- Physical security: lock computers when unattended

**Enhanced Detection:**
- Machine learning for typing pattern recognition
- Active prevention with kernel-level input filtering

---

## 7. Conclusion

### Key Findings

1. **Attack Simplicity**: USB HID injection is easy to implement
2. **OS Trust Model**: Operating systems inherently trust USB keyboards without authentication
3. **Detection Effectiveness**: Timing-based analysis provides 100% detection with 0% false positives
4. **Evasion Possibility**: Sophisticated attackers can evade detection with timing delays and obfuscation
5. **Defense in Depth Required**: Effective protection needs multiple layers (hardware, kernel, user-space)

### OS Security Insights

This project provided hands-on experience with:

**Kernel Development:**
- Loadable kernel modules provide powerful capabilities but require careful memory management
- USB subsystem demonstrates layered driver architecture
- Sysfs provides elegant kernel-to-userspace communication

**Device Interaction:**
- HID class driver automatically binds
- Input subsystem abstracts hardware details
- Device lifecycle management requires robust error handling

**Process Management:**
- User-space daemons require elevated privileges
- Event-driven architecture enables efficient I/O multiplexing
- Resource management is critical for long-running processes

### Technical Challenges Overcome

1. **Race Condition**: Used `inotify` for immediate device file detection
2. **Device Initialization**: Consecutive error counting prevented spurious disconnections
3. **Memory Safety**: Careful buffer management prevented kernel panics

---

## References

1. Linux USB subsystem documentation: https://www.kernel.org/doc/html/latest/driver-api/usb/
2. USB HID specification: https://www.usb.org/hid
3. Arduino Keyboard library: https://www.arduino.cc/reference/en/language/functions/usb/keyboard/
4. Linux input subsystem: https://www.kernel.org/doc/Documentation/input/

---