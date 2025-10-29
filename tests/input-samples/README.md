# Input Sample Data

This directory stores recorded keystroke patterns for testing and analysis.

## File Types

- **legitimate_*.log**: Recorded legitimate typing sessions
- **injection_*.log**: Recorded injection attack patterns
- **evtest_*.log**: Raw evtest output for analysis

## Recording Samples

### Using evtest
```bash
sudo evtest /dev/input/event2 > legitimate_typing.log
```

### Using Python evdev
Create scripts to record and timestamp input events with metadata.

## Usage

These samples help with:
- Training detection thresholds
- Testing false positive rates
- Analyzing timing characteristics
- Debugging detection algorithms