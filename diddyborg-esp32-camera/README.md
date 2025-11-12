# DiddyBorg ESP32-S3 Camera Board

Optional camera module for DiddyBorg robot with live streaming and SD card recording.

## Features

✅ **Live MJPEG Streaming** (800x600 @ 15-30fps)
✅ **SD Card Recording** with automatic rotation
✅ **UART Communication** with motor controller
✅ **Configurable Camera Settings** (brightness, contrast, quality, etc.)
✅ **Web Interface** for viewing and downloading recordings
✅ **Automatic File Management** (deletes oldest when >50% full)

## Hardware Requirements

- ESP32-S3 N16R8 CAM development board
- OV2640 or OV5640 camera module
- microSD card (32GB Class 10 recommended)
- 3x wires for UART connection to motor controller

## Wiring

### Connection to Motor Controller Board

```
Camera Board          Motor Controller
------------          ----------------
GPIO 17 (TX)   →      GPIO 18 (RX)
GPIO 18 (RX)   →      GPIO 17 (TX)
GND            →      GND (common ground!)
```

**CRITICAL**: Both ESP32 boards must share a common ground.

### Camera Module

Camera should be pre-connected to the ESP32-S3-CAM board. Most boards have the camera connector labeled.

### MicroSD Card

Insert microSD card into the slot on the ESP32-S3-CAM board (usually on bottom).

**Format**: FAT32 for cards ≤32GB, exFAT for larger cards.

## Software Setup

### 1. Configure WiFi

Edit `src/main.cpp` to match your motor controller's WiFi:

```cpp
#define WIFI_SSID       "DiddyBorg"        // Match motor controller AP
#define WIFI_PASSWORD   "diddyborg123"     // Match motor controller password
```

### 2. Configure Camera Pins (If Different Board)

If using a different ESP32-S3-CAM variant, check the board schematic and update pin definitions in `main.cpp`:

```cpp
#define CAM_PIN_D7      48
#define CAM_PIN_D6      11
// ... etc
```

Common boards:
- **ESP32-S3-EYE**: Pins already configured
- **Freenove ESP32-S3-CAM**: May need pin adjustment
- **Generic ESP32-S3 + camera**: Check your schematic

### 3. Build and Upload

```bash
cd diddyborg-esp32-camera

# Build firmware
pio run

# Upload to camera board
pio run --target upload

# Monitor serial output
pio device monitor
```

### 4. Verify Operation

Serial output should show:

```
======================================
  DiddyBorg ESP32-S3 Camera Board
======================================

UART initialized
Camera initialized successfully
SD Card: 32000MB
Connecting to WiFi: DiddyBorg
..........
Connected! IP: 192.168.4.2

=== Camera Board Ready ===
Stream: http://192.168.4.2:81/stream
==========================
```

## Usage

### Via Motor Controller Web UI

1. Connect to motor controller WiFi: `DiddyBorg`
2. Open `http://192.168.4.1` in browser
3. Camera section will show "Online" if connected
4. Click **"Open Stream"** to view live feed
5. Click **"Start Recording"** to save to SD card
6. Click **"Refresh Files"** to see recorded videos
7. Click **"Download"** to save recordings to your device

### Direct Access

Camera board also has its own endpoints:

- **Stream**: `http://[camera-ip]:81/stream`
- **Status**: `http://[camera-ip]:81/status`
- **Files**: `http://[camera-ip]:81/files`
- **Download**: `http://[camera-ip]:81/download?file=vid_12345.mjpg`

## Recording Details

### File Format

**MJPEG** (Motion JPEG) - simple concatenated JPEG frames
- Easy to work with
- Good quality
- ~5-10MB per minute @ 640x480

### File Naming

Files are named with timestamp: `vid_<millis>.mjpg`

Example: `vid_1234567.mjpg`

### Automatic Rotation

When SD card reaches 50% capacity:
1. Camera deletes oldest recording
2. Continues recording normally
3. Repeats as needed

**Example**: 32GB card
- 50% = 16GB used
- ~1600 minutes (26 hours) of video
- Oldest files deleted automatically

### Session Splitting

Recordings are split into **5-minute chunks** for:
- Manageable file sizes (~50MB per chunk)
- Easier downloading
- Prevents corruption if power lost

## Camera Settings

Adjustable via web UI or UART commands:

### Quality Settings
- **Quality**: 10-63 (lower = better, larger files)
- **Resolution**: VGA (640x480), SVGA (800x600), XGA (1024x768)

### Image Tuning
- **Brightness**: -2 to +2
- **Contrast**: -2 to +2
- **Saturation**: -2 to +2

### Advanced
- **White Balance**: Auto/manual
- **Exposure**: Auto/manual
- **Gain Control**: Auto/manual

## UART Protocol

Commands sent from motor controller:

| Command | Response | Description |
|---------|----------|-------------|
| `PING` | `PONG` | Check connection |
| `REC_START` | `OK` | Start recording |
| `REC_STOP` | `OK` | Stop recording |
| `STATUS` | `STATUS:<json>` | Get camera status |
| `FILES` | `FILES:<json>` | List recordings |
| `SET:key=value` | `OK` | Change setting |
| `DELETE:filename` | `OK` | Delete file |

Example status JSON:
```json
{
  "streaming": true,
  "recording": false,
  "sd_total": 32000,
  "sd_used": 4500,
  "sd_free": 27500,
  "file_count": 15,
  "ip": "192.168.4.2",
  "stream_port": 81
}
```

## Troubleshooting

### Camera Not Detected

**Symptoms**: Serial shows "Camera init failed!"

**Solutions**:
1. Check camera module connection (ribbon cable)
2. Verify pin configuration matches your board
3. Try different camera (OV2640 vs OV5640)
4. Check if PSRAM is available: `psramFound()` should return true

### SD Card Not Working

**Symptoms**: Serial shows "SD Card mount failed"

**Solutions**:
1. Format card as FAT32 (≤32GB) or exFAT (>32GB)
2. Try different SD card (Class 10 or higher)
3. Check SD card slot connection
4. Verify SD pins not conflicting with camera

### WiFi Won't Connect

**Symptoms**: "WiFi connection failed"

**Solutions**:
1. Verify SSID/password matches motor controller
2. Check motor controller WiFi AP is running
3. Move boards closer together
4. Check for antenna issues (if external antenna port)

### UART Not Communicating

**Symptoms**: Motor controller shows "Camera board not detected"

**Solutions**:
1. Verify TX/RX wires (TX→RX, RX→TX crossover)
2. Check common ground connection
3. Verify baud rate (115200) matches both boards
4. Try swap TX/RX if backwards

### Stream is Slow/Choppy

**Solutions**:
1. Reduce resolution: VGA instead of SVGA
2. Increase quality number (lower quality = faster)
3. Check WiFi signal strength
4. Disable recording while streaming

### SD Card Fills Up Too Fast

**Solutions**:
1. Increase quality number (smaller files)
2. Reduce resolution
3. Lower SD_ROTATION_PERCENT to 30%
4. Use larger SD card

## Performance

### Streaming

| Resolution | FPS | Network Load |
|------------|-----|--------------|
| VGA (640x480) | 20-30 | ~1-2 Mbps |
| SVGA (800x600) | 15-25 | ~2-4 Mbps |
| XGA (1024x768) | 10-15 | ~3-5 Mbps |

### Recording

| Setting | File Size (per minute) | SD Card Capacity (32GB @ 50%) |
|---------|----------------------|------------------------------|
| VGA, Q12 | ~10MB | ~26 hours |
| SVGA, Q12 | ~15MB | ~17 hours |
| VGA, Q20 | ~5MB | ~53 hours |

### Power Consumption

- Idle: ~80mA
- Streaming: ~160mA
- Recording: ~200mA
- **Total with motor controller**: ~440mA (electronics only)

## Customization

### Change Recording Chunk Length

Edit `main.cpp`:

```cpp
#define RECORDING_CHUNK_MINUTES     10  // Change from 5 to 10 minutes
```

### Change SD Rotation Threshold

```cpp
#define SD_ROTATION_PERCENT         30  // Delete at 30% instead of 50%
```

### Change Stream Port

```cpp
#define STREAM_PORT                 8080  // Use port 8080 instead of 81
```

### Change Resolution

In `initCamera()` function:

```cpp
config.frame_size = FRAMESIZE_VGA;  // Options: VGA, SVGA, XGA, SXGA, etc.
```

## File Structure

```
diddyborg-esp32-camera/
├── platformio.ini          # Build configuration
├── README.md               # This file
└── src/
    └── main.cpp           # Main firmware
```

## Tested Configurations

✅ ESP32-S3-DevKitC-1 + OV2640
✅ ESP32-S3-EYE board
⚠️ ESP32-CAM (original) - NOT compatible (needs ESP32-S3)
⚠️ OV5640 - Works but slower FPS

## Future Enhancements

Possible additions (not yet implemented):

- [ ] H.264 encoding (smaller files, more CPU)
- [ ] Motion detection recording
- [ ] Timestamp overlay on video
- [ ] Pan/tilt servo control
- [ ] Audio recording (if board has microphone)
- [ ] Timelapse mode

## License

MIT License - Same as main DiddyBorg project

## Support

For camera-specific issues, check:
1. Serial monitor output
2. Motor controller web UI camera status
3. Direct camera board web interface

For hardware issues, consult your ESP32-S3-CAM board documentation.
