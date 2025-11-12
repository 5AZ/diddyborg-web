/**
 * DiddyBorg ESP32-S3 Camera Board
 *
 * Features:
 * - MJPEG streaming web server
 * - SD card recording with rotation
 * - UART communication with motor controller
 * - Configurable camera settings
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_camera.h>
#include <FS.h>
#include <SD_MMC.h>
#include <ArduinoJson.h>

// ===========================================
// CONFIGURATION
// ===========================================

// WiFi settings (join same network as motor controller)
#define WIFI_SSID       "DiddyBorg"
#define WIFI_PASSWORD   "diddyborg123"

// UART pins for communication with motor controller
#define UART_TX         17
#define UART_RX         18
#define UART_BAUD       115200

// Recording settings
#define RECORDING_CHUNK_MINUTES     5       // Split recordings into 5-minute chunks
#define SD_ROTATION_PERCENT         50      // Start deleting when 50% full
#define RECORDING_FOLDER            "/recordings"

// Stream settings
#define STREAM_PORT                 81

// Camera pins for ESP32-S3-CAM (adjust if different)
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    10
#define CAM_PIN_SIOD    40
#define CAM_PIN_SIOC    39

#define CAM_PIN_D7      48
#define CAM_PIN_D6      11
#define CAM_PIN_D5      12
#define CAM_PIN_D4      14
#define CAM_PIN_D3      16
#define CAM_PIN_D2      18
#define CAM_PIN_D1      17
#define CAM_PIN_D0      15
#define CAM_PIN_VSYNC   38
#define CAM_PIN_HREF    47
#define CAM_PIN_PCLK    13

// ===========================================
// GLOBAL VARIABLES
// ===========================================

AsyncWebServer server(STREAM_PORT);
HardwareSerial SerialUART(1);

bool cameraInitialized = false;
bool sdCardAvailable = false;
bool recording = false;

File recordingFile;
unsigned long recordingStartTime = 0;
unsigned long lastFrameTime = 0;
uint32_t frameCount = 0;

// Camera settings
sensor_t* cameraSensor = nullptr;

// ===========================================
// CAMERA INITIALIZATION
// ===========================================

bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sscb_sda = CAM_PIN_SIOD;
    config.pin_sscb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // High quality settings if PSRAM available
    if (psramFound()) {
        config.frame_size = FRAMESIZE_SVGA;  // 800x600
        config.jpeg_quality = 12;             // Lower = better (10-63)
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_VGA;   // 640x480
        config.jpeg_quality = 15;
        config.fb_count = 1;
    }

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    // Get camera sensor for settings adjustments
    cameraSensor = esp_camera_sensor_get();

    // Apply default settings
    if (cameraSensor) {
        cameraSensor->set_brightness(cameraSensor, 0);     // -2 to 2
        cameraSensor->set_contrast(cameraSensor, 0);       // -2 to 2
        cameraSensor->set_saturation(cameraSensor, 0);     // -2 to 2
        cameraSensor->set_special_effect(cameraSensor, 0); // 0 = none
        cameraSensor->set_whitebal(cameraSensor, 1);       // 0 = disable, 1 = enable
        cameraSensor->set_awb_gain(cameraSensor, 1);       // 0 = disable, 1 = enable
        cameraSensor->set_wb_mode(cameraSensor, 0);        // 0 = auto
        cameraSensor->set_exposure_ctrl(cameraSensor, 1);  // 0 = disable, 1 = enable
        cameraSensor->set_aec2(cameraSensor, 0);           // 0 = disable, 1 = enable
        cameraSensor->set_ae_level(cameraSensor, 0);       // -2 to 2
        cameraSensor->set_aec_value(cameraSensor, 300);    // 0 to 1200
        cameraSensor->set_gain_ctrl(cameraSensor, 1);      // 0 = disable, 1 = enable
        cameraSensor->set_agc_gain(cameraSensor, 0);       // 0 to 30
        cameraSensor->set_gainceiling(cameraSensor, (gainceiling_t)0); // 0 to 6
        cameraSensor->set_bpc(cameraSensor, 0);            // 0 = disable, 1 = enable
        cameraSensor->set_wpc(cameraSensor, 1);            // 0 = disable, 1 = enable
        cameraSensor->set_raw_gma(cameraSensor, 1);        // 0 = disable, 1 = enable
        cameraSensor->set_lenc(cameraSensor, 1);           // 0 = disable, 1 = enable
        cameraSensor->set_hmirror(cameraSensor, 0);        // 0 = disable, 1 = enable
        cameraSensor->set_vflip(cameraSensor, 0);          // 0 = disable, 1 = enable
        cameraSensor->set_dcw(cameraSensor, 1);            // 0 = disable, 1 = enable
        cameraSensor->set_colorbar(cameraSensor, 0);       // 0 = disable, 1 = enable
    }

    Serial.println("Camera initialized successfully");
    cameraInitialized = true;
    return true;
}

// ===========================================
// SD CARD
// ===========================================

bool initSDCard() {
    if (!SD_MMC.begin("/sdcard", true)) {  // 1-bit mode
        Serial.println("SD Card mount failed");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
        return false;
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD Card: %lluMB\n", cardSize);

    // Create recordings folder
    if (!SD_MMC.exists(RECORDING_FOLDER)) {
        SD_MMC.mkdir(RECORDING_FOLDER);
    }

    sdCardAvailable = true;
    return true;
}

uint64_t getSDUsedSpace() {
    return SD_MMC.usedBytes() / (1024 * 1024);  // MB
}

uint64_t getSDTotalSpace() {
    return SD_MMC.cardSize() / (1024 * 1024);   // MB
}

uint64_t getSDFreeSpace() {
    return (SD_MMC.cardSize() - SD_MMC.usedBytes()) / (1024 * 1024);
}

int getFileCount() {
    File root = SD_MMC.open(RECORDING_FOLDER);
    if (!root || !root.isDirectory()) {
        return 0;
    }

    int count = 0;
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            count++;
        }
        file = root.openNextFile();
    }
    return count;
}

void deleteOldestFile() {
    File root = SD_MMC.open(RECORDING_FOLDER);
    if (!root || !root.isDirectory()) {
        return;
    }

    String oldestFile = "";
    time_t oldestTime = LONG_MAX;

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            time_t mtime = file.getLastWrite();
            if (mtime < oldestTime) {
                oldestTime = mtime;
                oldestFile = String(file.path());
            }
        }
        file = root.openNextFile();
    }

    if (oldestFile.length() > 0) {
        Serial.printf("Deleting oldest file: %s\n", oldestFile.c_str());
        SD_MMC.remove(oldestFile.c_str());
    }
}

void checkSDRotation() {
    if (!sdCardAvailable) return;

    uint64_t used = getSDUsedSpace();
    uint64_t total = getSDTotalSpace();

    if (total > 0 && (used * 100 / total) > SD_ROTATION_PERCENT) {
        Serial.println("SD card over 50%, deleting oldest file");
        deleteOldestFile();
    }
}

// ===========================================
// RECORDING
// ===========================================

void startRecording() {
    if (!sdCardAvailable || recording) return;

    checkSDRotation();  // Check before starting

    // Generate filename with timestamp
    char filename[64];
    snprintf(filename, sizeof(filename), "%s/vid_%lu.mjpg",
             RECORDING_FOLDER, millis());

    recordingFile = SD_MMC.open(filename, FILE_WRITE);
    if (!recordingFile) {
        Serial.println("Failed to create recording file");
        return;
    }

    recording = true;
    recordingStartTime = millis();
    frameCount = 0;

    Serial.printf("Recording started: %s\n", filename);
}

void stopRecording() {
    if (!recording) return;

    if (recordingFile) {
        recordingFile.close();
    }

    recording = false;
    Serial.printf("Recording stopped. Frames: %u\n", frameCount);
}

void recordFrame(camera_fb_t* fb) {
    if (!recording || !recordingFile || !fb) return;

    // Write MJPEG frame (just concatenate JPEGs)
    recordingFile.write(fb->buf, fb->len);
    frameCount++;

    // Check if chunk time exceeded
    if ((millis() - recordingStartTime) >= (RECORDING_CHUNK_MINUTES * 60 * 1000)) {
        Serial.println("Recording chunk complete, starting new file");
        stopRecording();
        startRecording();
    }
}

// ===========================================
// WEB SERVER
// ===========================================

void handleStream(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginChunkedResponse("multipart/x-mixed-replace;boundary=frame",
        [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
            if (!cameraInitialized) return 0;

            camera_fb_t* fb = esp_camera_fb_get();
            if (!fb) return 0;

            // Record frame if recording
            recordFrame(fb);

            // Build multipart HTTP response
            static bool headerSent = false;
            static size_t frameOffset = 0;

            if (!headerSent) {
                String header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " +
                               String(fb->len) + "\r\n\r\n";

                size_t headerLen = header.length();
                if (headerLen <= maxLen) {
                    memcpy(buffer, header.c_str(), headerLen);
                    headerSent = true;
                    frameOffset = 0;
                    return headerLen;
                }
            }

            // Send frame data
            size_t remaining = fb->len - frameOffset;
            size_t toSend = (remaining < maxLen) ? remaining : maxLen;

            memcpy(buffer, fb->buf + frameOffset, toSend);
            frameOffset += toSend;

            if (frameOffset >= fb->len) {
                esp_camera_fb_return(fb);
                headerSent = false;
                frameOffset = 0;

                // Add frame separator
                const char* separator = "\r\n";
                memcpy(buffer + toSend, separator, 2);
                return toSend + 2;
            }

            return toSend;
        });

    request->send(response);
}

void handleStatus(AsyncWebServerRequest* request) {
    StaticJsonDocument<512> doc;

    doc["streaming"] = true;
    doc["recording"] = recording;
    doc["sd_total"] = getSDTotalSpace();
    doc["sd_used"] = getSDUsedSpace();
    doc["sd_free"] = getSDFreeSpace();
    doc["file_count"] = getFileCount();
    doc["ip"] = WiFi.localIP().toString();
    doc["stream_port"] = STREAM_PORT;

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleFiles(AsyncWebServerRequest* request) {
    StaticJsonDocument<2048> doc;
    JsonArray files = doc.to<JsonArray>();

    File root = SD_MMC.open(RECORDING_FOLDER);
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                JsonObject fileObj = files.createNestedObject();
                fileObj["name"] = String(file.name());
                fileObj["size"] = file.size();
            }
            file = root.openNextFile();
        }
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleDownload(AsyncWebServerRequest* request) {
    if (request->hasParam("file")) {
        String filename = request->getParam("file")->value();
        String path = String(RECORDING_FOLDER) + "/" + filename;

        if (SD_MMC.exists(path.c_str())) {
            request->send(SD_MMC, path.c_str(), "video/x-motion-jpeg", true);
            return;
        }
    }
    request->send(404, "text/plain", "File not found");
}

void setupWebServer() {
    server.on("/stream", HTTP_GET, handleStream);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/files", HTTP_GET, handleFiles);
    server.on("/download", HTTP_GET, handleDownload);

    server.begin();
    Serial.printf("Web server started on port %d\n", STREAM_PORT);
}

// ===========================================
// UART COMMUNICATION
// ===========================================

void handleUARTCommand(String command) {
    command.trim();

    if (command == "PING") {
        SerialUART.println("PONG");
    }
    else if (command == "REC_START") {
        startRecording();
        SerialUART.println("OK");
    }
    else if (command == "REC_STOP") {
        stopRecording();
        SerialUART.println("OK");
    }
    else if (command == "STATUS") {
        StaticJsonDocument<512> doc;
        doc["streaming"] = true;
        doc["recording"] = recording;
        doc["sd_total"] = getSDTotalSpace();
        doc["sd_used"] = getSDUsedSpace();
        doc["sd_free"] = getSDFreeSpace();
        doc["file_count"] = getFileCount();
        doc["ip"] = WiFi.localIP().toString();
        doc["stream_port"] = STREAM_PORT;

        String json;
        serializeJson(doc, json);

        SerialUART.print("STATUS:");
        SerialUART.println(json);
    }
    else if (command == "FILES") {
        // Send file list (simplified)
        SerialUART.print("FILES:");
        SerialUART.println("[]");  // TODO: Full implementation
    }
    else if (command.startsWith("SET:")) {
        String setting = command.substring(4);
        int eqPos = setting.indexOf('=');
        if (eqPos > 0 && cameraSensor) {
            String key = setting.substring(0, eqPos);
            int value = setting.substring(eqPos + 1).toInt();

            // Apply camera setting
            if (key == "brightness") cameraSensor->set_brightness(cameraSensor, value);
            else if (key == "contrast") cameraSensor->set_contrast(cameraSensor, value);
            else if (key == "saturation") cameraSensor->set_saturation(cameraSensor, value);
            else if (key == "quality") cameraSensor->set_quality(cameraSensor, value);
            else if (key == "framesize") cameraSensor->set_framesize(cameraSensor, (framesize_t)value);

            SerialUART.println("OK");
        } else {
            SerialUART.println("ERROR");
        }
    }
    else {
        SerialUART.println("ERROR");
    }
}

void processUART() {
    static String buffer = "";

    while (SerialUART.available()) {
        char c = SerialUART.read();
        if (c == '\n') {
            handleUARTCommand(buffer);
            buffer = "";
        } else {
            buffer += c;
        }
    }
}

// ===========================================
// SETUP & LOOP
// ===========================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n======================================");
    Serial.println("  DiddyBorg ESP32-S3 Camera Board");
    Serial.println("======================================\n");

    // Initialize UART
    SerialUART.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
    Serial.println("UART initialized");

    // Initialize camera
    if (!initCamera()) {
        Serial.println("Camera init failed!");
        while (1) { delay(1000); }
    }

    // Initialize SD card
    if (!initSDCard()) {
        Serial.println("SD Card init failed (recording disabled)");
    }

    // Connect to WiFi
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
        setupWebServer();
    } else {
        Serial.println("\nWiFi connection failed");
    }

    Serial.println("\n=== Camera Board Ready ===");
    Serial.printf("Stream: http://%s:%d/stream\n", WiFi.localIP().toString().c_str(), STREAM_PORT);
    Serial.println("==========================\n");
}

void loop() {
    processUART();

    // Periodic SD card rotation check
    static unsigned long lastRotationCheck = 0;
    if (millis() - lastRotationCheck > 60000) {  // Every minute
        lastRotationCheck = millis();
        if (recording) {
            checkSDRotation();
        }
    }

    delay(10);
}
