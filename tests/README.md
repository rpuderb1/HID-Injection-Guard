# HID Guard Testing

This directory contains test tools for validating the HID injection detection system.

## Test Files

### 1. Automated Unit Tests (`test_detector`)

Validates pattern matching logic

**Build:**
```bash
make
```

**Run:**
```bash
./test_detector
```

**Test:**
- ✅ 14 different command patterns
- ✅ Scoring calculations
- ✅ All pattern detection types (download, obfuscation, execution, persistence, chaining)

---

### 2. Arduino HID Injection Testing
```bash
# Load kernel module
cd ../kernel
sudo insmod usb_monitor.ko

# Start daemon
cd ../daemon
sudo ./hid_guard

# Plug in Arduino with malicious payload
# Observe: Fast IKT (~1-5ms), pattern detection alerts
```

---

## Cleaning Up

```bash
make clean
```
