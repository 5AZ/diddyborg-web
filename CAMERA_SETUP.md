# Camera Integration Guide

Complete guide for adding the optional ESP32-S3 camera module to your DiddyBorg robot.

## Overview

The camera system uses a **second ESP32-S3 board** dedicated to camera operations:

```
┌─────────────────────────┐      ┌──────────────────────────┐
│  ESP32-S3 DevKit        │      │  ESP32-S3-CAM            │
│  (Motor Controller)     │◄────►│  (Camera + Recording)    │
│                         │ UART │                          │
│  - Gamepad/Flysky input │ WiFi │  - Live MJPEG streaming  │
│  - PicoBorgRev motors   │      │  - SD card recording     │
│  - Web configuration UI │      │  - Camera settings       │
└─────────────────────────┘      └──────────────────────────┘
         |                                    |
         |                                    |
    [Gamepad/RC]                         [Camera]
```

## Why Two Boards?

**Reliability**: Motor control stays fast and responsive even while streaming video
**Modularity**: Use camera only when needed (saves power)
**Debugging**: Each system can be tested independently
**Flexibility**: Upgrade camera without touching motor controller

## What You Need

### Hardware

- [ ] ESP32-S3 N16R8 CAM board (the one you ordered) ✅
- [ ] OV2640 camera module (recommended) or OV5640
- [ ] microSD card: 32GB Class 10
- [ ] 3x jumper wires (for UART connection)
- [ ] Camera mount (3D printed or improvised)

### Estimated Cost

- ESP32-S3-CAM: ~$10-15
- OV2640 camera: Usually included with board
- microSD card 32GB: ~$8
- **Total**: ~$18-23

## Physical Integration

### Step 1: Mount Camera Board

**Options**:

**A. Front Mount** (simplest):
- Hot glue or zip-tie camera board to front of robot
- Ensure lens has clear view forward
- Protect from debris (consider plastic cover)

**B. Top Mount** (better view):
- Mount on top of robot
- Angle slightly downward (10-15°)
- Better for navigation/FPV

**C. Pan/Tilt Mount** (future):
- Add servo mount later
- For now, just static forward-facing

### Step 2: Wire UART Connection

```
Camera Board Pin    Wire Color    Motor Board Pin
----------------    ----------    ---------------
GPIO 17 (TX)    →   Yellow    →   GPIO 18 (RX)
GPIO 18 (RX)    →   Green     →   GPIO 17 (TX)
GND             →   Black     →   GND
```

**Wire Management**:
- Keep wires short (~10-15cm)
- Avoid motor wires (EMI interference)
- Secure with zip ties or hot glue

### Step 3: Power

**Camera board needs 5V power**:

**Option A**: Separate USB power bank
- Simplest for testing
- Camera board stays powered independently
- Pro: Easy debugging
- Con: Extra battery to manage

**Option B**: Share robot's 12V battery (needs voltage regulator)
- Add 12V→5V buck converter (LM2596 module ~$2)
- Connect to same battery as motors
- Pro: Single battery system
- Con: Needs additional component

**Recommendation**: Start with Option A (USB power bank) for testing.

## Software Setup

### Step 1: Flash Motor Controller (Main Board)

**Already done!** The code you have includes camera support.

Just reflash to get latest changes:

```bash
cd diddyborg-esp32
pio run --target upload
```

### Step 2: Flash Camera Board

```bash
cd diddyborg-esp32-camera

# First time: install dependencies
pio run

# Upload to camera board
pio run --target upload

# Monitor output
pio device monitor
```

**Expected output**:
```
Camera initialized successfully
SD Card: 32000MB
Connecting to WiFi: DiddyBorg
Connected! IP: 192.168.4.2
Stream: http://192.168.4.2:81/stream
```

### Step 3: Test Connection

1. **Power on motor controller** first (it creates WiFi AP)
2. **Power on camera board** (connects to motor controller's WiFi)
3. **Connect your phone/laptop** to `DiddyBorg` WiFi
4. **Open web UI**: `http://192.168.4.1`
5. **Camera section** should show **(Online)**

## Using the Camera

### Web Interface

**Main Control Panel** (`http://192.168.4.1`):
- Shows camera status
- Stream embed
- Recording controls
- File download
- Camera settings sliders

**Direct Stream** (`http://192.168.4.2:81/stream`):
- Full-screen stream
- Lower latency
- No UI overlay

### Recording

**Start Recording**:
1. Click "Start Recording" in web UI
2. Red dot appears (recording indicator)
3. Files saved to SD card in 5-minute chunks

**Stop Recording**:
1. Click "Stop Recording"
2. Files remain on SD card

**Download Recordings**:
1. Click "Refresh Files"
2. Click "Download" next to desired file
3. File downloads as `.mjpg` (plays in VLC, MPV, etc.)

### Camera Settings

Adjust in web UI:

**Brightness** (-2 to +2): For darker/lighter environments
**Contrast** (-2 to +2): Enhance details
**Saturation** (-2 to +2): Color intensity
**Resolution**: VGA (faster) or SVGA (better quality)
**Quality**: 10-20 range (lower = better, larger files)

**Tip**: Start with defaults, adjust if needed.

## Common Scenarios

### Scenario 1: Quick Test Drive with View

1. Power both boards
2. Connect to `DiddyBorg` WiFi on your phone
3. Open `http://192.168.4.1` in browser
4. Click "Open Stream" (opens in new tab)
5. Drive with gamepad while watching stream

### Scenario 2: Record a Session

1. Start driving
2. Click "Start Recording" in web UI
3. Drive around for 5-10 minutes
4. Click "Stop Recording"
5. Click "Refresh Files"
6. Download video to computer

### Scenario 3: FPV Driving

1. Mount phone/tablet on transmitter (if using Flysky)
2. Open stream in browser
3. Drive while watching screen
4. **Note**: ~200ms latency, not for racing!

## Troubleshooting

### "Camera (Offline)" in Web UI

**Check**:
1. Camera board powered on?
2. Camera board WiFi connected? (check serial monitor)
3. UART wires connected correctly?
4. Common ground between boards?

**Solution**: Check serial monitors of both boards for errors.

### Stream is Laggy

**Try**:
1. Reduce resolution to VGA
2. Increase quality number (lower quality = faster)
3. Move closer to WiFi AP
4. Stop recording while streaming

### SD Card Not Detected

**Check**:
1. Card formatted as FAT32
2. Card properly inserted
3. Card not corrupted (test on computer)

**Solution**: Try different SD card, reformat.

### Camera Image Upside Down

**Fix** in camera board `main.cpp`:

```cpp
cameraSensor->set_vflip(cameraSensor, 1);  // Vertical flip
cameraSensor->set_hmirror(cameraSensor, 1); // Horizontal mirror
```

Reflash camera board.

## Performance Expectations

### Streaming

- **Latency**: 200-500ms (not real-time)
- **FPS**: 15-25 fps @ VGA
- **Range**: ~10m from WiFi AP
- **Use case**: Casual viewing, not racing

### Recording

- **Quality**: Good (MJPEG)
- **Duration**: ~26 hours on 32GB card (VGA, Q12)
- **Playback**: Works in VLC, MPV, most video players

### Battery Life

With camera:
- **Electronics**: ~440mA (both ESP32s)
- **Motors**: 1-5A depending on load
- **Typical**: 30-90 minutes of mixed driving

Without camera (motor controller only):
- **Electronics**: ~240mA
- **Motors**: Same
- **Typical**: 45-120 minutes

## Future Enhancements

Things you can add later:

### Easy Additions
- [ ] Pan/tilt servo mount
- [ ] Better camera case/protection
- [ ] Voltage regulator for shared battery
- [ ] Larger SD card (64GB+)

### Advanced
- [ ] Motion detection recording
- [ ] FPV goggle analog video out
- [ ] Object detection (TensorFlow Lite)
- [ ] Autonomous navigation

## Tips & Tricks

**Tip 1**: Test camera separately before integration
- Flash camera board
- Test stream on phone
- Confirm recording works
- Then mount to robot

**Tip 2**: Use large SD card
- 64GB or 128GB if available
- exFAT format for >32GB
- Gives weeks of recording capacity

**Tip 3**: Backup important recordings
- Camera auto-deletes at 50% full
- Download any keepers to computer
- SD card is not permanent storage

**Tip 4**: Adjust for your environment
- Indoor: Increase brightness
- Outdoor: Use default settings
- Low light: Reduce frame rate, increase exposure

**Tip 5**: Power management
- Camera uses ~200mA always
- Turn off camera board when not needed
- Save battery for longer drives

## Wiring Diagram (Complete System)

```
                    ┌─────────────────┐
                    │   12V Battery   │
                    └────────┬────────┘
                             │
              ┌──────────────┴──────────────┐
              │                             │
              ▼                             ▼
    ┌──────────────────┐          ┌─────────────────┐
    │  PicoBorgRev     │          │  Buck Converter │
    │  Motor Driver    │          │  12V → 5V       │
    └────────┬─────────┘          └────────┬────────┘
             │ I2C                         │
             │                             │
    ┌────────▼─────────┐          ┌────────▼────────┐
    │  ESP32-S3 DevKit │◄────────►│ ESP32-S3-CAM    │
    │  Motor Control   │  UART    │ Camera Board    │
    │  GPIO 21/22 I2C  │          │ GPIO 17/18 UART │
    │  GPIO 17/18 UART │          │ Camera Module   │
    └──────────────────┘          │ SD Card         │
             │                     └─────────────────┘
             │
             ▼
    ┌──────────────────┐
    │  Gamepad/RC RX   │
    └──────────────────┘
```

## Summary

**Benefits**:
- ✅ See where robot is going
- ✅ Record drives for review
- ✅ Impressive demo feature
- ✅ Optional (robot works without it)

**Drawbacks**:
- ⚠️ Adds ~$20 cost
- ⚠️ Extra setup complexity
- ⚠️ Uses more power
- ⚠️ Not real-time (200ms+ latency)

**Recommendation**:
1. Get driving working first ✅
2. Add camera when you're comfortable with system
3. Test camera separately before mounting
4. Start with USB power bank for simplicity

**Ready to add camera?** Follow this guide step-by-step, and you'll have FPV driving in ~2-3 hours!
