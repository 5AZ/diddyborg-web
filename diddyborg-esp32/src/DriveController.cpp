/**
 * DriveController.cpp
 *
 * Differential steering implementation
 */

#include "DriveController.h"

DriveController::DriveController(PicoBorgRev* motorController) {
    _motorController = motorController;
    _driveMode = DRIVE_MODE_ARCADE;
    _speedLimit = 1.0f;
    _deadzone = 0.1f;
    _rampingEnabled = true;
    _rampRate = 3.0f;  // Full power in 0.33 seconds
    _invertLeft = false;
    _invertRight = false;

    _currentLeft = 0.0f;
    _currentRight = 0.0f;
    _targetLeft = 0.0f;
    _targetRight = 0.0f;
    _lastUpdateTime = millis();
}

void DriveController::setDrive(float left, float right) {
    // Apply deadzone
    left = applyDeadzone(left);
    right = applyDeadzone(right);

    // Apply speed limit
    left = applySpeedLimit(left);
    right = applySpeedLimit(right);

    // Apply inversion if needed
    if (_invertLeft) left = -left;
    if (_invertRight) right = -right;

    if (_rampingEnabled) {
        // Set as target, will ramp in update()
        _targetLeft = left;
        _targetRight = right;
    } else {
        // Apply immediately
        _currentLeft = left;
        _currentRight = right;

        // Send to motors (note: Motor2 needs negation for proper direction)
        _motorController->setMotor1(_currentRight);
        _motorController->setMotor2(-_currentLeft);
    }
}

void DriveController::setArcadeDrive(float throttle, float steering) {
    // Apply deadzone first
    throttle = applyDeadzone(throttle);
    steering = applyDeadzone(steering);

    // Arcade mixing: combine throttle and steering
    float left = throttle + steering;
    float right = throttle - steering;

    // Normalize if values exceed limits
    float maxMagnitude = max(abs(left), abs(right));
    if (maxMagnitude > 1.0f) {
        left /= maxMagnitude;
        right /= maxMagnitude;
    }

    setDrive(left, right);
}

void DriveController::stop() {
    _targetLeft = 0.0f;
    _targetRight = 0.0f;
    _currentLeft = 0.0f;
    _currentRight = 0.0f;
    _motorController->motorsOff();
}

void DriveController::setSpeedLimit(float limit) {
    _speedLimit = constrain(limit, 0.0f, 1.0f);
}

void DriveController::setDeadzone(float deadzone) {
    _deadzone = constrain(deadzone, 0.0f, 0.5f);
}

void DriveController::setRamping(bool enabled) {
    _rampingEnabled = enabled;
    if (!enabled) {
        // If disabling, immediately apply current targets
        _currentLeft = _targetLeft;
        _currentRight = _targetRight;
    }
}

void DriveController::update() {
    if (!_rampingEnabled) return;

    unsigned long currentTime = millis();
    float deltaTime = (currentTime - _lastUpdateTime) / 1000.0f;
    _lastUpdateTime = currentTime;

    // Prevent huge jumps if loop stalls
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    // Ramp towards target values
    _currentLeft = rampTowards(_currentLeft, _targetLeft, deltaTime);
    _currentRight = rampTowards(_currentRight, _targetRight, deltaTime);

    // Send to motors (note: Motor2 needs negation for proper direction)
    _motorController->setMotor1(_currentRight);
    _motorController->setMotor2(-_currentLeft);
}

// Private helper methods

float DriveController::applyDeadzone(float value) {
    if (abs(value) < _deadzone) {
        return 0.0f;
    }

    // Scale the remaining range to 0-1
    float sign = (value > 0) ? 1.0f : -1.0f;
    float scaledValue = (abs(value) - _deadzone) / (1.0f - _deadzone);
    return sign * scaledValue;
}

float DriveController::applySpeedLimit(float value) {
    return value * _speedLimit;
}

float DriveController::rampTowards(float current, float target, float deltaTime) {
    float maxChange = _rampRate * deltaTime;
    float difference = target - current;

    if (abs(difference) <= maxChange) {
        return target;  // Close enough, snap to target
    }

    if (difference > 0) {
        return current + maxChange;
    } else {
        return current - maxChange;
    }
}
