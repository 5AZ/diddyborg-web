/**
 * DiddyBorg ESP32-S3 Main Application
 *
 * Supports dual input methods:
 * 1. Bluetooth Gamepad (via Bluepad32)
 * 2. Flysky FS-i6 RC Receiver (PPM/PWM)
 *
 * Auto-detects input source or uses configured preference
 */

#include <Arduino.h>
#include <Wire.h>
#include <Bluepad32.h>
#include <Preferences.h>
#include "Config.h"
#include "PicoBorgRev.h"
#include "DriveController.h"
#include "FlyskyInput.h"
#include "CameraComm.h"
#include "WebServer.h"
#include "WebAuth.h"

// Pin Configuration
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define FLYSKY_PPM_PIN      19
#define STATUS_LED_PIN      2
#define CONFIG_BUTTON_PIN   0  // Boot button

// Input modes
enum InputMode {
    INPUT_MODE_AUTO,
    INPUT_MODE_GAMEPAD,
    INPUT_MODE_FLYSKY,
    INPUT_MODE_NONE
};

// Global objects
PicoBorgRev motorController;
DriveController* driveController = nullptr;
FlyskyInput flyskyInput;
CameraComm cameraComm;
WebAuth webAuth;
DiddyWebServer* webServer = nullptr;
Preferences preferences;

// State variables
InputMode currentMode = INPUT_MODE_NONE;
InputMode configuredMode = INPUT_MODE_AUTO;
bool gamepadConnected = false;
bool flyskyConnected = false;
unsigned long lastActivityTime = 0;
unsigned long lastStatusPrint = 0;

// Bluepad32 callbacks
GamepadPtr myGamepad = nullptr;

void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    gamepadConnected = true;
    Serial.println("Gamepad connected!");
    Serial.printf("Model: %s, VID:PID: %04x:%04x\n",
                  gp->getModelName().c_str(),
                  gp->getVendorId(),
                  gp->getProductId());
}

void onDisconnectedGamepad(GamepadPtr gp) {
    myGamepad = nullptr;
    gamepadConnected = false;
    Serial.println("Gamepad disconnected!");

    // Stop motors on disconnect
    if (driveController) {
        driveController->stop();
    }
}

// Setup functions
void setupI2C() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);  // 100kHz for reliability
    Serial.println("I2C initialized");
}

void setupMotorController() {
    Serial.println("Initializing PicoBorg Reverse...");

    if (motorController.begin(Wire)) {
        Serial.println("Motor controller ready!");
        driveController = new DriveController(&motorController);
        driveController->setSpeedLimit(0.7f);  // Start at 70% for safety
        driveController->setDeadzone(0.15f);
        driveController->setRamping(true);
    } else {
        Serial.println("FATAL: Motor controller not found!");
        while (1) {
            digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
            delay(100);  // Fast blink error
        }
    }
}

void setupBluePad32() {
    Serial.println("Initializing Bluepad32...");

    // Set Bluepad32 callbacks
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

    // Force gamepad mode (no mouse/keyboard)
    BP32.forceGamepadMode();

    Serial.println("Bluepad32 ready - waiting for gamepad...");
}

void setupFlysky() {
    Serial.println("Initializing Flysky receiver...");

    // Start in PPM mode (simpler, single pin)
    flyskyInput.beginPPM(FLYSKY_PPM_PIN);

    // Configure channel reversals if needed (adjust based on your TX setup)
    // flyskyInput.setChannelReverse(0, true);  // Reverse steering if needed

    Serial.println("Flysky receiver ready");
}

void loadConfiguration() {
    preferences.begin("diddyborg", false);

    // Load saved input mode preference
    uint8_t savedMode = preferences.getUChar("input_mode", INPUT_MODE_AUTO);
    configuredMode = (InputMode)savedMode;

    Serial.printf("Loaded config: Input mode = %d\n", configuredMode);

    preferences.end();
}

void saveConfiguration() {
    preferences.begin("diddyborg", false);
    preferences.putUChar("input_mode", configuredMode);
    preferences.end();

    Serial.println("Configuration saved");
}

// Input detection and mode selection
void updateInputMode() {
    static unsigned long modeCheckTime = 0;
    unsigned long now = millis();

    // Check for mode changes every 500ms
    if (now - modeCheckTime < 500) {
        return;
    }
    modeCheckTime = now;

    // Update connection status
    gamepadConnected = (myGamepad != nullptr && myGamepad->isConnected());
    flyskyInput.update();
    flyskyConnected = flyskyInput.isConnected();

    InputMode previousMode = currentMode;

    // Determine active mode based on configuration and connections
    switch (configuredMode) {
        case INPUT_MODE_AUTO:
            // Prefer gamepad if both connected
            if (gamepadConnected) {
                currentMode = INPUT_MODE_GAMEPAD;
            } else if (flyskyConnected) {
                currentMode = INPUT_MODE_FLYSKY;
            } else {
                currentMode = INPUT_MODE_NONE;
            }
            break;

        case INPUT_MODE_GAMEPAD:
            currentMode = gamepadConnected ? INPUT_MODE_GAMEPAD : INPUT_MODE_NONE;
            break;

        case INPUT_MODE_FLYSKY:
            currentMode = flyskyConnected ? INPUT_MODE_FLYSKY : INPUT_MODE_NONE;
            break;

        default:
            currentMode = INPUT_MODE_NONE;
            break;
    }

    // Log mode changes
    if (currentMode != previousMode) {
        Serial.printf("Input mode changed: ");
        switch (currentMode) {
            case INPUT_MODE_GAMEPAD:
                Serial.println("GAMEPAD");
                break;
            case INPUT_MODE_FLYSKY:
                Serial.println("FLYSKY");
                break;
            case INPUT_MODE_NONE:
                Serial.println("NONE");
                driveController->stop();
                break;
            default:
                break;
        }
    }

    // Stop motors if no input
    if (currentMode == INPUT_MODE_NONE && previousMode != INPUT_MODE_NONE) {
        driveController->stop();
    }
}

// Process gamepad input
void processGamepadInput() {
    if (!myGamepad || !myGamepad->isConnected()) return;

    // Update gamepad state
    myGamepad->update();

    // Read analog sticks (range: -512 to 512)
    int leftY = -myGamepad->axisY();     // Forward/back (invert for intuitive control)
    int leftX = myGamepad->axisX();      // Left/right strafe (not used in tank mode)
    int rightY = -myGamepad->axisRY();   // Right stick Y
    int rightX = myGamepad->axisRX();    // Right stick X (steering)

    // Normalize to -1.0 to 1.0
    float throttle = leftY / 512.0f;
    float steering = rightX / 512.0f;

    // Button controls
    if (myGamepad->a()) {
        // A button: Stop
        driveController->stop();
        return;
    }

    if (myGamepad->b()) {
        // B button: Speed boost (100%)
        driveController->setSpeedLimit(1.0f);
    } else if (myGamepad->y()) {
        // Y button: Slow mode (30%)
        driveController->setSpeedLimit(0.3f);
    } else {
        // Default: 70% speed
        driveController->setSpeedLimit(0.7f);
    }

    // Arcade mode: Left stick Y + Right stick X
    driveController->setArcadeDrive(throttle, steering);

    lastActivityTime = millis();
}

// Process Flysky RC input
void processFlyskyInput() {
    if (!flyskyConnected) return;

    flyskyInput.update();

    // Read channels (already normalized to -1.0 to 1.0)
    float throttle = flyskyInput.getThrottle();
    float steering = flyskyInput.getSteering();

    // Use arcade-style mixing
    driveController->setArcadeDrive(throttle, steering);

    lastActivityTime = millis();
}

// Status LED blink patterns
void updateStatusLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    unsigned long now = millis();

    uint16_t blinkInterval;

    switch (currentMode) {
        case INPUT_MODE_GAMEPAD:
            blinkInterval = 1000;  // Slow blink
            break;
        case INPUT_MODE_FLYSKY:
            blinkInterval = 250;   // Fast blink
            break;
        case INPUT_MODE_NONE:
            blinkInterval = 100;   // Very fast blink
            break;
        default:
            blinkInterval = 500;
            break;
    }

    if (now - lastBlink >= blinkInterval) {
        lastBlink = now;
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState);
    }
}

// Print status information
void printStatus() {
    unsigned long now = millis();
    if (now - lastStatusPrint < 2000) return;
    lastStatusPrint = now;

    Serial.println("=== Status ===");
    Serial.printf("Mode: %s | Gamepad: %s | Flysky: %s\n",
                  currentMode == INPUT_MODE_GAMEPAD ? "GAMEPAD" :
                  currentMode == INPUT_MODE_FLYSKY ? "FLYSKY" : "NONE",
                  gamepadConnected ? "YES" : "NO",
                  flyskyConnected ? "YES" : "NO");
    Serial.printf("Motors: L=%.2f R=%.2f | Speed Limit: %.0f%%\n",
                  driveController->getLeftPower(),
                  driveController->getRightPower(),
                  driveController->getSpeedLimit() * 100);
    Serial.println();
}

// Arduino setup
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=================================");
    Serial.println("  DiddyBorg ESP32-S3 Controller  ");
    Serial.println("=================================\n");

    // Initialize hardware
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);

    // Load configuration
    loadConfiguration();

    // Initialize subsystems
    setupI2C();
    setupMotorController();
    setupBluePad32();
    setupFlysky();

    // Initialize camera communication
    Serial.println("Initializing camera communication...");
    if (cameraComm.begin()) {
        Serial.println("Camera board connected!");
    } else {
        Serial.println("Camera board not detected (will retry in background)");
    }

    // Initialize authentication
    Serial.println("Initializing authentication...");
    webAuth.begin(DEFAULT_ACCESS_PIN);
    Serial.printf("Default PIN: %s (change this immediately!)\n", DEFAULT_ACCESS_PIN);

    // Start web server
    Serial.println("Starting web interface...");
    webServer = new DiddyWebServer(driveController, &cameraComm, &webAuth);
    if (webServer->begin()) {
        Serial.printf("Web UI: http://%s\n", webServer->getIPAddress().c_str());
    }

    Serial.println("\n=== System Ready ===");
    Serial.println("Waiting for input source...\n");

    lastActivityTime = millis();
}

// Arduino main loop
void loop() {
    // Update Bluepad32
    BP32.update();

    // Detect and switch input modes
    updateInputMode();

    // Process active input
    switch (currentMode) {
        case INPUT_MODE_GAMEPAD:
            processGamepadInput();
            break;

        case INPUT_MODE_FLYSKY:
            processFlyskyInput();
            break;

        case INPUT_MODE_NONE:
            // No input - motors already stopped in updateInputMode
            break;

        default:
            break;
    }

    // Update drive controller (for ramping)
    driveController->update();

    // Update camera communication
    cameraComm.update();

    // Update web server
    if (webServer) {
        webServer->update();
    }

    // Update status indicators
    updateStatusLED();

    // Print periodic status
    printStatus();

    // Safety timeout: stop motors if no input for 5 seconds
    if (millis() - lastActivityTime > 5000 && currentMode != INPUT_MODE_NONE) {
        Serial.println("TIMEOUT: No input for 5 seconds, stopping motors");
        driveController->stop();
        lastActivityTime = millis();  // Reset to avoid spam
    }

    // Small delay to prevent CPU hogging
    delay(10);
}
