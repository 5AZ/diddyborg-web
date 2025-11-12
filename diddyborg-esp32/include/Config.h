/**
 * Config.h
 *
 * User configuration for DiddyBorg ESP32 controller
 * Edit these values to customize behavior without modifying main code
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// ============================================

// I2C pins for PicoBorg Reverse
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define I2C_FREQUENCY       100000  // 100kHz (don't change unless you know what you're doing)

// Flysky RC receiver
#define FLYSKY_PPM_PIN      19      // PPM signal input pin
// For PWM mode, define individual channel pins:
// #define FLYSKY_CH1_PIN   19
// #define FLYSKY_CH2_PIN   18
// #define FLYSKY_CH3_PIN   17
// etc.

// Status indicators
#define STATUS_LED_PIN      2       // Built-in LED on most ESP32 boards
#define CONFIG_BUTTON_PIN   0       // Boot button (GPIO0)

// ============================================
// MOTOR CONTROLLER CONFIGURATION
// ============================================

// PicoBorg Reverse I2C address (default is 0x44)
#define PBR_I2C_ADDRESS     0x44

// Motor direction inversion (set to true if motors run backwards)
#define INVERT_LEFT_MOTOR   false
#define INVERT_RIGHT_MOTOR  false

// ============================================
// DRIVE CONTROLLER SETTINGS
// ============================================

// Speed limits (0.0 to 1.0)
#define DEFAULT_SPEED_LIMIT     0.7f    // 70% - safe default
#define BOOST_SPEED_LIMIT       1.0f    // 100% - when boost button pressed
#define SLOW_SPEED_LIMIT        0.3f    // 30% - when slow mode button pressed

// Control response
#define STICK_DEADZONE          0.15f   // Ignore stick movement below 15%
#define ENABLE_RAMPING          true    // Smooth acceleration on/off
#define RAMP_RATE               3.0f    // Units per second (higher = faster acceleration)

// ============================================
// INPUT CONFIGURATION
// ============================================

// Default input mode on startup
// Options: INPUT_MODE_AUTO, INPUT_MODE_GAMEPAD, INPUT_MODE_FLYSKY
#define DEFAULT_INPUT_MODE      INPUT_MODE_AUTO

// Gamepad settings
#define GAMEPAD_DEADZONE        0.15f   // Analog stick deadzone

// Flysky channel mapping (0-indexed)
// Standard Mode 2 mapping:
#define FLYSKY_THROTTLE_CH      2       // Channel 3 - Left stick Y
#define FLYSKY_STEERING_CH      0       // Channel 1 - Right stick X
#define FLYSKY_LEFT_STICK_CH    3       // Channel 4 - Right stick Y (for tank mode)
#define FLYSKY_RIGHT_STICK_CH   1       // Channel 2 - Left stick X (for tank mode)

// Flysky channel reversals (true to reverse direction)
#define FLYSKY_REVERSE_CH1      false
#define FLYSKY_REVERSE_CH2      false
#define FLYSKY_REVERSE_CH3      false
#define FLYSKY_REVERSE_CH4      false

// ============================================
// SAFETY SETTINGS
// ============================================

// Timeout (milliseconds) - stop motors if no input received
#define SAFETY_TIMEOUT_MS       5000    // 5 seconds

// Signal loss handling
#define ENABLE_SIGNAL_LOSS_STOP true    // Stop motors when controller disconnects

// EPO (Emergency Power Off) handling
#define EPO_IGNORE              false   // Set true if you don't have an EPO switch

// ============================================
// SECURITY SETTINGS
// ============================================

// Web UI access PIN (6-8 digits, user-changeable via UI)
#define DEFAULT_ACCESS_PIN      "123456"    // Change this default!

// Shared secret for device pairing (MUST match on both boards)
// Change this to a random string for your robot
// This prevents unauthorized devices from impersonating your camera
#define DEVICE_SHARED_SECRET    "DiddyBorg2024-SecretKey-ChangeMe!"

// Session timeout (seconds) - how long before re-entering PIN
#define SESSION_TIMEOUT_SECONDS 3600        // 1 hour

// ============================================
// DEBUG SETTINGS
// ============================================

// Serial debug output
#define SERIAL_BAUD_RATE        115200
#define ENABLE_STATUS_PRINT     true    // Print status every 2 seconds
#define ENABLE_VERBOSE_DEBUG    false   // Extra debug output (slows down loop)

// ============================================
// ADVANCED SETTINGS
// ============================================

// LED blink patterns (milliseconds)
#define LED_BLINK_GAMEPAD       1000    // Slow blink for gamepad mode
#define LED_BLINK_FLYSKY        250     // Fast blink for Flysky mode
#define LED_BLINK_NO_INPUT      100     // Very fast blink for no input

// Drive mode
// Options: DRIVE_MODE_TANK, DRIVE_MODE_ARCADE, DRIVE_MODE_RACING
#define DEFAULT_DRIVE_MODE      DRIVE_MODE_ARCADE

#endif // CONFIG_H
