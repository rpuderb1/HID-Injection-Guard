# Test Scripts

Automation scripts for testing the HID injection detection system.

## Scripts

- **run_tests.sh**: Main test runner that executes all test cases
- **load_system.sh**: Loads kernel module and starts daemon
- **unload_system.sh**: Stops daemon and unloads kernel module
- **analyze_logs.sh**: Parses logs and generates detection statistics
- **benchmark.sh**: Performance benchmarking script
- **simulate_typing.py**: Simulates human typing patterns for false positive testing

## Test Workflow

```bash
# Setup
./load_system.sh

# Run tests
./run_tests.sh

# Analyze results
./analyze_logs.sh

# Cleanup
./unload_system.sh
```