# USB HID Keystroke Injection Project - Progress Report

**Team Members:** Rylan & Gideon  
**Course:** COSC 439: Operating   
**Project:** USB HID Keystroke Injection: Attack Vector and OS-Level Defense Mechanisms

---

## Components Identified

1. **Arduino HID Attack Vector** - Firmware that enumerates as USB keyboard and executes keystroke payloads
2. **Linux Kernel Module** - Monitors USB bus, detects HID devices, exposes info via sysfs
3. **User-Space Daemon** - Analyzes keystroke patterns to detect injection attacks
4. **Testing Framework** - Validates detection accuracy with attack and benign scenarios

---

## Tools & Libraries

**Development:**
- GCC, Make, Arduino CLI, Git
- Linux kernel headers

**Kernel APIs:**
- `usb_register_notify()` - USB device event notifications
- `kobject_create_and_add()` - Sysfs interface
- `usb_string()` - USB descriptor retrieval

**Userspace:**
- Linux input subsystem (`/dev/input/eventX`, `struct input_event`)
- Standard C library (stdio, stdlib, string, unistd)

**Hardware:**
- Arduino Micro (ATmega32U4) with Keyboard.h library
- Arduino CLI for firmware upload

**Testing:**
- evtest, lsusb, dmesg, strace

---

## Progress Made

### 1. Arduino HID Attack Vector (100% Complete)
- Firmware successfully enumerates as USB keyboard
- Executes pre-programmed keystroke sequences
- Tested with multiple payload types

**Challenges Solved:**
- USB enumeration timing (added delay after `Keyboard.begin()`)

### 2. Linux Kernel Module (100% Complete)
- Monitors USB bus using a notifier chain
- Detects HID devices (class 0x03 at interface level)
- Extracts VID, PID, manufacturer, product, serial
- Exposes data via `/sys/kernel/usb_hid_monitor/`

**Challenges Solved:**
- Null pointer dereferences (added proper null checking and `usb_get_dev()` reference counting)
- HID detection (iterate interfaces, not just device class)
- Buffer overflows (used `snprintf()` for safe string handling)
- Sysfs memory management (proper `kobject_put()` cleanup)

### 3. User-Space Daemon (65% Complete)
**Completed:**
- Basic input monitoring - opens `/dev/input/event*`, reads raw events, parses keystrokes
- Dynamic device detection - uses inotify to detect new devices in real-time, handles disconnections gracefully
- Timing analysis - calculates IKT (Inter-Keystroke Timing), jitter (standard deviation), displays periodic statistics
- Command buffering - reconstructs complete commands with shift/caps lock state tracking, character mapping
- Kernel module integration - reads USB device identification (VID/PID/manufacturer/product/serial) from sysfs for newly connected devices

**Remaining:**
- Pattern matching for malicious commands (curl, wget, base64, eval, persistence, etc.)
- Attack chain detection (Download→Execute→Persist sequences)
- Composite scoring and alerting

**Challenges Solved:**
- **Race condition with HID injection timing:**
  - Arduino payloads execute within 2-4 seconds of power-on
  - Arduino's `delay()` starts counting at power-on, not when device file appears
  - Only a small window of time to detect, open, and start monitoring device before payload executes
  - Solution: Use inotify to detect device file creation immediately, open with zero delay

- **Spurious device initialization errors:**
  - Opening `/dev/input/eventX` immediately after creation often returns POLLERR/POLLHUP
  - Device not fully initialized yet, but will become ready within 1-2 poll cycles
  - Solution: Consecutive error counter - ignore first 3 errors (transient), remove after 4+ (real disconnect)
  - Prevents premature device removal while allowing graceful cleanup on real disconnects

- **Kernel module single-device limitation:**
  - Kernel module stores only the LAST connected HID device in global variables
  - Reading sysfs for all existing devices at startup would mislabel them all identically
  - Solution: Only read sysfs when new device connects (attack vector)

---

## Task Distribution

**Rylan:**
- Arduino firmware development and testing
- Kernel module (notifier registration, descriptor parsing, HID filtering, sysfs creation, testing)
- Daemon foundation (input monitoring, keystroke parsing)
- Daemon core: Timing analysis, command buffering, device trust
- Research (HID injection techniques, detection strategies)
 
**Gideon:**

- Pattern detection: Pattern database, command matching, attack chain detection, scoring/alerts
- Testing framework: Test scripts, false positive measurement

**Both:**
- Arduino payload development (4-5 attack scenarios)
- End-to-end integration testing
- Presentation preparation: Slides, live demo, talking points
- Technical report and architecture diagrams
- Final review