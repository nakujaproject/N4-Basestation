# Communication Mode Configuration

The base station supports multiple communication methods between your laptop and the ESP32. You can manually control which method to use.

## Available Communication Methods

### 1. **Bluetooth SPP** (via HC-05/HC-06)
- **Pros**: Wireless, no USB cable needed
- **Cons**: Can be finicky, requires pairing and power cycling
- **Best for**: Field operations, desk testing without cables

### 2. **USB Serial** (via USB cable)
- **Pros**: More reliable, faster, no pairing needed
- **Cons**: Requires USB cable
- **Best for**: Development, debugging, indoor testing

## Command-Line Flags

### Auto-Detect Mode (Default)
```bash
python start_basestation_integrated.py
```
- Prefers Bluetooth if detected
- Falls back to USB Serial if Bluetooth unavailable
- **Use when**: You want the system to choose automatically

### Force USB Serial Only
```bash
python start_basestation_integrated.py --force-usb
```
- Ignores Bluetooth completely
- Only uses USB Serial connection
- **Use when**: 
  - Bluetooth is unreliable
  - You prefer wired connection
  - Debugging communication issues

### Force Bluetooth Only
```bash
python start_basestation_integrated.py --force-bluetooth
```
- Ignores USB Serial completely
- Only uses Bluetooth connection
- **Use when**: 
  - USB port unavailable
  - Testing wireless operation
  - Preparing for field deployment

### Simulation Mode (No Hardware)
```bash
python start_basestation_integrated.py --simulation
```
- No real hardware needed
- Generates simulated telemetry
- **Use when**: 
  - Testing software without hardware
  - Developing UI features
  - Demonstrating system

## Environment Variables

You can also set preferences via environment variables:

### Force USB Serial
```bash
# Windows PowerShell
$env:N4_FORCE_USB = "1"
python start_basestation_integrated.py

# Windows CMD
set N4_FORCE_USB=1
python start_basestation_integrated.py
```

### Force Bluetooth
```bash
# Windows PowerShell
$env:N4_FORCE_BT = "1"
python start_basestation_integrated.py

# Windows CMD
set N4_FORCE_BT=1
python start_basestation_integrated.py
```

### Specify Exact COM Port
```bash
# Windows PowerShell
$env:N4_COM_PORT = "COM7"
python start_basestation_integrated.py

# Windows CMD
set N4_COM_PORT=COM7
python start_basestation_integrated.py
```

## Troubleshooting

### Bluetooth Not Working
1. **Try forcing USB**:
   ```bash
   python start_basestation_integrated.py --force-usb
   ```
2. If this works, your Bluetooth module may need:
   - Re-pairing
   - Power cycling
   - Baud rate adjustment

### USB Not Working
1. **Try forcing Bluetooth**:
   ```bash
   python start_basestation_integrated.py --force-bluetooth
   ```
2. If this works, your USB connection may have:
   - Driver issues
   - Port conflicts
   - Cable problems

### Neither Working
1. **Check hardware**:
   - ESP32 powered on?
   - Bluetooth module connected?
   - USB cable is data cable (not charge-only)?
2. **Try simulation mode** to test software:
   ```bash
   python start_basestation_integrated.py --simulation
   ```

## Typical Usage Patterns

### Development (Desk Testing)
```bash
# Use USB for reliability
python start_basestation_integrated.py --force-usb
```

### Field Testing (Wireless)
```bash
# Use Bluetooth for wireless operation
python start_basestation_integrated.py --force-bluetooth
```

### Auto Mode (Most Flexible)
```bash
# Let system decide (Bluetooth preferred)
python start_basestation_integrated.py
```

### Demo/Training (No Hardware)
```bash
# Use simulation
python start_basestation_integrated.py --simulation
```

## Server Configuration

The communication preference is handled in [research/server.py](research/server.py):

```python
# === COMMUNICATION METHOD PREFERENCE ===
FORCE_USB_SERIAL = False  # True = USB only
FORCE_BLUETOOTH = False   # True = Bluetooth only
```

You can edit these directly in the file if you want to set a permanent preference without using command-line flags.

## Status Messages

When starting the base station, you'll see the communication mode:

```
🔄 Communication Mode: Auto-detect (Bluetooth preferred, USB fallback)
```

or

```
🔌 Communication Mode: FORCED USB Serial (Bluetooth disabled)
```

or

```
📡 Communication Mode: FORCED Bluetooth (USB disabled)
```

This confirms which method is active.
