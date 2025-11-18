# Testing and Validation

This directory contains test scripts and sample data for validating the detection system.

## Structure

- **scripts/**: Test automation scripts

## Test Categories

### 1. Detection Accuracy Tests
- Verify detection of various injection payloads
- Measure false positive rate with legitimate typing
- Test edge cases and boundary conditions

### 2. Timing Analysis Tests
- Analyze detection time window
- Measure response latency
- Test with varying keystroke speeds

### 3. Evasion Technique Tests
- Test payloads with added delays
- Verify detection of sophisticated attacks
- Measure effectiveness of timing randomization

### 4. Performance Tests
- CPU and memory usage monitoring
- Impact on system responsiveness
- Scalability with multiple devices

## Testing Approach

Test the system by:
1. Loading the kernel module (`sudo insmod kernel/usb_monitor.ko`)
2. Starting the daemon (`sudo daemon/hid_guard`)
3. Connecting the Arduino device with attack payload
4. Monitoring kernel logs (`dmesg`) and daemon output
5. Observing detection behavior and timing statistics