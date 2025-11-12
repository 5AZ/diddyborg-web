# Quick Start Guide - 15 Minutes to Driving!

This guide gets you from zero to driving robot in ~15 minutes.

## What You Need

**Hardware** (check your box):
- [ ] ESP32-S3 board
- [ ] USB-C cable
- [ ] Your DiddyBorg robot (with PicoBorg Reverse installed)
- [ ] Charged battery
- [ ] PlayStation/Xbox controller OR Flysky FS-i6

**Software**:
- [ ] Computer with USB port
- [ ] Python 3 installed

## Step 1: Install PlatformIO (2 minutes)

**Easy Way** - Install PlatformIO Core:

```bash
# Linux/Mac
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/get-platformio.py -o get-platformio.py
python3 get-platformio.py

# Add to PATH (Linux/Mac)
export PATH=$PATH:~/.platformio/penv/bin

# Windows - use PowerShell as Administrator
python -m pip install platformio
```

**Alternative** - Install VS Code + PlatformIO IDE extension (graphical, easier for beginners)

## Step 2: Wire the ESP32 (5 minutes)

### Minimal Wiring (Just Motor Control)

```
ESP32-S3 Pin    →    PicoBorg Reverse Pin
-------------         --------------------
GPIO 21 (SDA)   →    SDA (usually marked or pin 3)
GPIO 22 (SCL)   →    SCL (usually marked or pin 5)
GND             →    GND (any ground pin)
```

**CRITICAL**: ESP32 and PicoBorg Reverse **MUST** share a common ground!

**Power**:
- ESP32: Power via USB during testing
- PicoBorg: Connect your 12V battery as normal
- Robot motors: Already wired to PicoBorg

### Optional: Add Status LED

```
ESP32 GPIO 2  →  220Ω resistor  →  LED (+)  →  LED (-)  →  GND
```

### Optional: Add Flysky Receiver

```
ESP32 GPIO 19  →  Flysky IA6 PPM pin (or CH1)
ESP32 GND      →  Flysky IA6 GND
```

## Step 3: Flash the Firmware (3 minutes)

```bash
# Navigate to project
cd diddyborg-web/diddyborg-esp32

# Build and upload (this will auto-install libraries)
pio run --target upload

# This will:
# 1. Download Bluepad32 library (~30 seconds)
# 2. Compile code (~1 minute)
# 3. Upload to ESP32 (~30 seconds)
```

**First time?** It downloads ~200MB of tools. Be patient!

**Troubleshooting**:
- "No device found": Check USB cable, try different port
- "Permission denied": Linux users run `sudo pio run --target upload`
- "Error 2": Press BOOT button on ESP32, then retry upload

## Step 4: Verify It's Working (1 minute)

Open serial monitor:

```bash
pio device monitor
```

You should see:

```
=================================
  DiddyBorg ESP32-S3 Controller
=================================

PBR: Initializing PicoBorg Reverse at 0x44
PBR: Found PicoBorg Reverse at 0x44
Motor controller ready!
Bluepad32 ready - waiting for gamepad...
Flysky receiver ready

=== System Ready ===
Waiting for input source...
```

If you see "FATAL: Motor controller not found!":
- ❌ Check wiring (especially SDA/SCL/GND)
- ❌ Verify PicoBorg is powered
- ❌ Try swapping SDA/SCL wires

## Step 5: Pair Your Controller (2 minutes)

### Option A: PlayStation Controller

1. Hold **SHARE + PS button** until light bar flashes white rapidly
2. Wait ~10 seconds
3. Serial monitor shows: `Gamepad connected! Model: PS4`
4. Light bar turns solid blue

**Still flashing?**
- Turn OFF other Bluetooth devices nearby
- Move controller closer to ESP32
- Try again

### Option B: Xbox Controller

1. Hold **Pairing button** (top of controller) until Xbox logo flashes rapidly
2. Wait ~10 seconds
3. Serial monitor shows: `Gamepad connected! Model: Xbox`

### Option C: Flysky RC

1. Turn on FS-i6 transmitter
2. Serial monitor shows: `Input mode changed: FLYSKY`
3. Move sticks - you should see activity in serial monitor

## Step 6: DRIVE! (2 minutes)

### Safety First!

1. **Put robot on blocks** (wheels off ground) for first test
2. **Verify motor directions** are correct
3. **Start at low speed** (Y/Triangle button for 30% mode)

### Controls - PlayStation/Xbox

- **Left Stick**: Forward/Backward
- **Right Stick**: Left/Right steering
- **A/Cross**: STOP (emergency)
- **Y/Triangle**: Slow mode (30%)
- **B/Circle**: Speed boost (100%)

### First Drive Test

1. Gently push left stick forward
2. Wheels should spin forward
3. Push right stick left
4. Left wheels should slow down (robot turns left)
5. Press A/Cross to stop

**Wrong direction?** See "Step 7: Troubleshooting" below

### Put Robot on Ground

Once you've verified:
- ✅ Motors spin correct direction
- ✅ Steering works as expected
- ✅ Emergency stop works

Place robot on ground and drive!

## Step 7: Common Issues

### Motors Spin Backwards

Edit `diddyborg-esp32/include/Config.h`:

```cpp
#define INVERT_LEFT_MOTOR   true   // Change to true
#define INVERT_RIGHT_MOTOR  true   // Change to true
```

Re-upload: `pio run --target upload`

### Robot Spins Instead of Going Straight

One motor is wired backwards. Either:
- Physically swap the motor wires on PicoBorg, OR
- Invert just that motor in `Config.h`

### Robot is Too Fast/Slow

Edit `Config.h`:

```cpp
#define DEFAULT_SPEED_LIMIT     0.5f    // Change from 0.7 to 0.5 (50%)
```

### Controller Won't Pair

1. Turn OFF Bluetooth on your phone/computer
2. Reset controller (hold PS+SHARE for 10 seconds until light goes off)
3. Try pairing again
4. Still broken? Try different controller or use Flysky

### "No PicoBorg Reverse found"

Check wiring:
```
ESP32 GPIO21 (SDA) → PicoBorg SDA pin
ESP32 GPIO22 (SCL) → PicoBorg SCL pin
ESP32 GND → PicoBorg GND
```

Verify PicoBorg has power (LED should be on).

Try swapping SDA/SCL if still not working.

## Step 8: Go Play!

You're done! Your robot now:
- ✅ Boots in 3 seconds
- ✅ Auto-connects to controller
- ✅ Has smooth controls
- ✅ Stops on signal loss

### Tips for Best Experience

**Battery Life**:
- ESP32 uses ~240mA
- Will run for hours on a decent battery
- Motors drain battery much faster than ESP32

**Range**:
- Bluetooth gamepad: ~10 meters
- Flysky RC: ~500+ meters

**Storage**:
- Leave ESP32 powered via USB when not in use
- Bluetooth pairing is remembered
- No SD card to corrupt!

## Next Steps

Want to customize?
- Read `README.md` for full documentation
- Edit `Config.h` for easy tweaks
- Modify `main.cpp` for advanced changes

Want to add camera?
- See README section "Advanced: Adding Camera Support Later"

## Need Help?

1. Check serial monitor output (`pio device monitor`)
2. Read `README.md` troubleshooting section
3. Create an issue on GitHub with:
   - Serial monitor output
   - What you tried
   - What happened vs what you expected

---

**Congratulations!** You've successfully modernized your DiddyBorg! 🎉🤖
