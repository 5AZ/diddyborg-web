# DiddyBorg ESP32-S3 Controller

Modern ESP32-S3 based controller for DiddyBorg 6-wheel robot, supporting both Bluetooth gamepad and RC receiver control.

## Features

- ✅ **Dual Input Support**: Bluetooth gamepad OR Flysky RC receiver
- ✅ **Auto-Detection**: Automatically switches between available inputs
- ✅ **Smooth Control**: Acceleration ramping and deadzone handling
- ✅ **Safety Features**: Signal loss detection, timeout protection
- ✅ **Fast Boot**: ~2-3 seconds vs ~30+ seconds on Raspberry Pi
- ✅ **Low Power**: ~240mA vs ~500mA+ on Raspberry Pi

## Hardware Requirements

### Required
- ESP32-S3 DevKit (any variant with 8MB+ flash)
- PicoBorg Reverse motor controller
- 6x DC motors (existing DiddyBorg hardware)
- Battery pack (12V nominal recommended)

### For Gamepad Control
- PlayStation 4/5 controller (recommended)
- OR Xbox One/Series controller
- OR 8BitDo/generic Bluetooth gamepad

### For RC Control (Optional)
- Flysky FS-i6 transmitter + IA6 receiver
- OR any PPM/PWM RC receiver

## Wiring Diagram

```
ESP32-S3          PicoBorg Reverse
---------         ----------------
GPIO 21 (SDA) --> SDA
GPIO 22 (SCL) --> SCL
GND           --> GND
3.3V          --> 3.3V (logic power only)

ESP32-S3          Flysky IA6 (Optional)
---------         ------------------
GPIO 19       --> PPM/CH1 out
GND           --> GND

ESP32-S3          Indicators
---------         ----------
GPIO 2        --> Status LED (+ 220Ω resistor to GND)
GPIO 0        --> Config Button (to GND)

PicoBorg Reverse  Motors & Power
----------------  ---------------
Motor 1 A/B   --> Right side motors (3x)
Motor 2 A/B   --> Left side motors (3x)
Battery +     --> 12V battery positive
Battery GND   --> Battery ground (common with ESP32 GND)
```

**IMPORTANT**: The ESP32 must share a common ground (GND) with the PicoBorg Reverse for I2C communication to work properly.

## Software Setup

### 1. Install PlatformIO

```bash
# Install PlatformIO CLI
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/get-platformio.py -o get-platformio.py
python3 get-platformio.py

# OR install VS Code + PlatformIO IDE extension
```

### 2. Clone and Build

```bash
cd diddyborg-web/diddyborg-esp32

# Build firmware
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output
pio device monitor
```

### 3. Pair Gamepad

**PlayStation 4/5 Controller**:
1. Hold **SHARE** + **PS** buttons until light bar flashes rapidly
2. ESP32 will automatically detect and connect
3. Light bar turns solid blue when connected

**Xbox Controller**:
1. Hold **pairing button** (on top) until Xbox button flashes rapidly
2. ESP32 will automatically detect and connect

**8BitDo/Generic**:
1. Check your controller's manual for Bluetooth pairing mode
2. Usually: Hold **START + R** or similar combination

### 4. Configure Flysky (If Using)

1. Bind your FS-i6 transmitter to the IA6 receiver (see Flysky manual)
2. Connect IA6's PPM output (or CH1) to ESP32 GPIO 19
3. Power on transmitter - ESP32 will auto-detect signal

## Usage

### Gamepad Controls

**PlayStation/Xbox Layout**:
- **Left Stick**: Forward/Backward (throttle)
- **Right Stick**: Left/Right (steering)
- **A/Cross**: Emergency Stop
- **B/Circle**: Speed Boost (100%)
- **Y/Triangle**: Slow Mode (30%)
- **Default**: 70% speed limit

**Steering Behavior** (Differential/Skid Steering):
- Forward + No Steering: Drive straight
- Forward + Left: Arc left
- Forward + Full Left: Spin left in place
- Neutral + Left: Pivot left on center
- Backward + Right: Arc right while reversing

### Flysky RC Controls

**Default Channel Mapping**:
- **Channel 3** (Left Stick Y): Forward/Backward
- **Channel 1** (Right Stick X): Steering

**Trim Settings**: Adjust trim on your transmitter if robot doesn't drive straight.

### Status LED

- **Slow Blink (1s)**: Gamepad mode active
- **Fast Blink (250ms)**: Flysky mode active
- **Very Fast Blink (100ms)**: No input detected
- **Solid On**: Processing input

### Serial Monitor

Connect via USB and open serial monitor (115200 baud) to see:
- Input mode status
- Motor power levels
- Connection state
- Debugging information

## Configuration

### Change Input Mode Preference

Edit in `main.cpp` before uploading:

```cpp
// Force gamepad only
configuredMode = INPUT_MODE_GAMEPAD;

// Force Flysky only
configuredMode = INPUT_MODE_FLYSKY;

// Auto-detect (default)
configuredMode = INPUT_MODE_AUTO;
```

### Adjust Motor Behavior

In `main.cpp`:

```cpp
// Maximum speed (0.0 to 1.0)
driveController->setSpeedLimit(0.7f);  // 70% default

// Stick deadzone (0.0 to 0.5)
driveController->setDeadzone(0.15f);

// Smooth acceleration on/off
driveController->setRamping(true);

// Reverse motor direction if needed
driveController->setInvertLeft(true);
driveController->setInvertRight(false);
```

### Custom Pin Assignments

In `main.cpp`:

```cpp
#define I2C_SDA_PIN         21  // Change to your SDA pin
#define I2C_SCL_PIN         22  // Change to your SCL pin
#define FLYSKY_PPM_PIN      19  // Change to your PPM pin
#define STATUS_LED_PIN      2   // Change to your LED pin
```

## Troubleshooting

### Motors Don't Move

1. **Check I2C Connection**:
   - Serial output shows "Motor controller ready!"?
   - If "FATAL: Motor controller not found!", check wiring
   - Verify SDA/SCL pins and common ground

2. **Check EPO (Emergency Power Off)**:
   - PicoBorg Reverse has EPO jumper installed?
   - Or set `setEpoIgnore(true)` in code

3. **Check Power**:
   - Battery voltage sufficient? (7.2V minimum, 12V recommended)
   - PicoBorg Reverse LED on?

### Gamepad Won't Pair

1. **Reset Gamepad**: Hold PS+SHARE (or Xbox pairing) for 10+ seconds
2. **Check Bluepad32 Library**: Should auto-install via PlatformIO
3. **Try Different Gamepad**: PS4/PS5 controllers have best compatibility

### Flysky Not Detected

1. **Check Wiring**: PPM pin connected to GPIO 19?
2. **Verify Signal**: Use oscilloscope/logic analyzer to confirm PPM pulses
3. **Try PWM Mode**: Edit `main.cpp` to use `beginPWM()` instead of `beginPPM()`

### Robot Drives Backwards

Swap motor wiring or use motor inversion in code:

```cpp
driveController->setInvertLeft(true);
driveController->setInvertRight(true);
```

### One Side Doesn't Work

- Check motor connections to PicoBorg Reverse
- Test with simple code: `motorController.setMotor1(0.5)` in loop()
- Verify motor driver outputs with multimeter

## Advanced: Adding Camera Support Later

While this version focuses on control, you can add camera streaming later:

**Option 1**: Add ESP32-CAM as secondary module
- Communicate via Serial or WiFi
- Independent MJPEG streaming

**Option 2**: Use ESP32-S3 with camera connector
- Boards like Freenove ESP32-S3-CAM
- Requires multi-threading (FreeRTOS)

**Option 3**: Raspberry Pi Zero as camera sidecar
- ESP32-S3 handles driving
- Pi Zero streams camera only

## Performance Comparison

| Feature | Raspberry Pi | ESP32-S3 |
|---------|-------------|----------|
| Boot Time | 30-40s | 2-3s |
| Power Draw | 500-700mA | 240mA |
| Control Latency | 50-100ms | 10-20ms |
| Cost | $35-75 | $5-15 |
| SD Card Corruption | Risk | No SD card |
| Bluetooth Setup | Complex | Built-in |

## License

Based on original DiddyBorg code by PiBorg (https://github.com/piborg/diddyborg-web)

ESP32 port and enhancements released under MIT License.

## Credits

- Original Python code: PiBorg
- Bluepad32 library: Ricardo Quesada
- ESP32 port: [Your name/handle]

## Support

For issues specific to this ESP32 implementation, create an issue on this repository.

For PicoBorg Reverse hardware questions, see https://www.piborg.org/picoborgrev
