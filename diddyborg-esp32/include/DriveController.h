/**
 * DriveController.h
 *
 * Unified differential (skid) steering controller
 * Handles motor mixing and speed limiting
 */

#ifndef DRIVE_CONTROLLER_H
#define DRIVE_CONTROLLER_H

#include <Arduino.h>
#include "PicoBorgRev.h"

// Drive modes
enum DriveMode {
    DRIVE_MODE_TANK,      // Left stick = left wheels, Right stick = right wheels
    DRIVE_MODE_ARCADE,    // Left stick Y = throttle, Right stick X = steering
    DRIVE_MODE_RACING     // Trigger for throttle, stick for steering
};

class DriveController {
public:
    DriveController(PicoBorgRev* motorController);

    // High-level control methods
    void setDrive(float left, float right);  // Direct left/right control (-1.0 to 1.0)
    void setArcadeDrive(float throttle, float steering);  // Arcade style control
    void stop();

    // Configuration
    void setDriveMode(DriveMode mode) { _driveMode = mode; }
    void setSpeedLimit(float limit);  // 0.0 to 1.0
    void setDeadzone(float deadzone); // 0.0 to 0.5
    void setRamping(bool enabled);    // Enable smooth acceleration
    void setInvertLeft(bool invert) { _invertLeft = invert; }
    void setInvertRight(bool invert) { _invertRight = invert; }

    // Status
    float getSpeedLimit() const { return _speedLimit; }
    float getLeftPower() const { return _currentLeft; }
    float getRightPower() const { return _currentRight; }

    // Update loop (call regularly for ramping)
    void update();

private:
    PicoBorgRev* _motorController;
    DriveMode _driveMode;

    // Settings
    float _speedLimit;
    float _deadzone;
    bool _rampingEnabled;
    bool _invertLeft;
    bool _invertRight;

    // Current state
    float _currentLeft;
    float _currentRight;
    float _targetLeft;
    float _targetRight;

    // Ramping parameters
    float _rampRate;  // Units per second
    unsigned long _lastUpdateTime;

    // Helper functions
    float applyDeadzone(float value);
    float applySpeedLimit(float value);
    float rampTowards(float current, float target, float deltaTime);
};

#endif // DRIVE_CONTROLLER_H
