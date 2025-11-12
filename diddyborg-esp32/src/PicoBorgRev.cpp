/**
 * PicoBorgRev.cpp
 *
 * ESP32 implementation of PicoBorg Reverse motor controller
 */

#include "PicoBorgRev.h"

PicoBorgRev::PicoBorgRev(uint8_t address) {
    _address = address;
    _foundChip = false;
    _wire = nullptr;
}

bool PicoBorgRev::begin(TwoWire &wirePort) {
    _wire = &wirePort;

    // Initialize I2C if not already done
    if (!_wire) {
        Serial.println("PBR: Wire interface is null!");
        return false;
    }

    Serial.printf("PBR: Initializing PicoBorg Reverse at 0x%02X\n", _address);

    // Check if the chip is present by reading its ID
    uint8_t buffer[PBR_I2C_MAX_LEN];
    if (rawRead(PBR_CMD_GET_ID, buffer, PBR_I2C_MAX_LEN)) {
        if (buffer[1] == PBR_I2C_ID) {
            _foundChip = true;
            Serial.printf("PBR: Found PicoBorg Reverse at 0x%02X\n", _address);

            // Initialize safety settings
            setCommsFailsafe(false);  // Disable failsafe initially
            resetEpo();               // Clear any EPO state
            motorsOff();              // Ensure motors are off

            return true;
        } else {
            Serial.printf("PBR: Device at 0x%02X is not PicoBorg Reverse (ID: 0x%02X)\n",
                         _address, buffer[1]);
        }
    } else {
        Serial.printf("PBR: No response from 0x%02X\n", _address);
    }

    _foundChip = false;
    return false;
}

bool PicoBorgRev::isConnected() {
    return _foundChip;
}

void PicoBorgRev::setMotor1(float power) {
    if (!_foundChip) return;

    power = clampPower(power);
    uint8_t command;
    uint8_t pwm;

    if (power < 0) {
        command = PBR_CMD_SET_B_REV;
        pwm = (uint8_t)(-power * PBR_PWM_MAX);
    } else {
        command = PBR_CMD_SET_B_FWD;
        pwm = (uint8_t)(power * PBR_PWM_MAX);
    }

    rawWrite(command, &pwm, 1);
}

void PicoBorgRev::setMotor2(float power) {
    if (!_foundChip) return;

    power = clampPower(power);
    uint8_t command;
    uint8_t pwm;

    if (power < 0) {
        command = PBR_CMD_SET_A_REV;
        pwm = (uint8_t)(-power * PBR_PWM_MAX);
    } else {
        command = PBR_CMD_SET_A_FWD;
        pwm = (uint8_t)(power * PBR_PWM_MAX);
    }

    rawWrite(command, &pwm, 1);
}

void PicoBorgRev::setMotors(float power) {
    if (!_foundChip) return;

    power = clampPower(power);
    uint8_t command;
    uint8_t pwm;

    if (power < 0) {
        command = PBR_CMD_SET_ALL_REV;
        pwm = (uint8_t)(-power * PBR_PWM_MAX);
    } else {
        command = PBR_CMD_SET_ALL_FWD;
        pwm = (uint8_t)(power * PBR_PWM_MAX);
    }

    rawWrite(command, &pwm, 1);
}

void PicoBorgRev::motorsOff() {
    if (!_foundChip) return;

    uint8_t data = 0;
    rawWrite(PBR_CMD_ALL_OFF, &data, 1);
}

void PicoBorgRev::setLed(bool state) {
    if (!_foundChip) return;

    uint8_t value = state ? PBR_VALUE_ON : PBR_VALUE_OFF;
    rawWrite(PBR_CMD_SET_LED, &value, 1);
}

bool PicoBorgRev::getLed() {
    if (!_foundChip) return false;

    uint8_t buffer[PBR_I2C_MAX_LEN];
    if (rawRead(PBR_CMD_GET_LED, buffer, PBR_I2C_MAX_LEN)) {
        return (buffer[1] == PBR_VALUE_ON);
    }
    return false;
}

void PicoBorgRev::resetEpo() {
    if (!_foundChip) return;

    uint8_t data = 0;
    rawWrite(PBR_CMD_RESET_EPO, &data, 1);
}

bool PicoBorgRev::getEpo() {
    if (!_foundChip) return false;

    uint8_t buffer[PBR_I2C_MAX_LEN];
    if (rawRead(PBR_CMD_GET_EPO, buffer, PBR_I2C_MAX_LEN)) {
        return (buffer[1] == PBR_VALUE_ON);
    }
    return false;
}

void PicoBorgRev::setEpoIgnore(bool state) {
    if (!_foundChip) return;

    uint8_t value = state ? PBR_VALUE_ON : PBR_VALUE_OFF;
    rawWrite(PBR_CMD_SET_EPO_IGNORE, &value, 1);
}

void PicoBorgRev::setCommsFailsafe(bool state) {
    if (!_foundChip) return;

    uint8_t value = state ? PBR_VALUE_ON : PBR_VALUE_OFF;
    rawWrite(PBR_CMD_SET_FAILSAFE, &value, 1);
}

bool PicoBorgRev::getDriveFault() {
    if (!_foundChip) return false;

    uint8_t buffer[PBR_I2C_MAX_LEN];
    if (rawRead(PBR_CMD_GET_DRIVE_FAULT, buffer, PBR_I2C_MAX_LEN)) {
        return (buffer[1] == PBR_VALUE_ON);
    }
    return false;
}

void PicoBorgRev::printStatus() {
    Serial.println("=== PicoBorg Reverse Status ===");
    Serial.printf("Connected: %s\n", _foundChip ? "Yes" : "No");
    Serial.printf("I2C Address: 0x%02X\n", _address);

    if (_foundChip) {
        Serial.printf("LED State: %s\n", getLed() ? "ON" : "OFF");
        Serial.printf("EPO Tripped: %s\n", getEpo() ? "YES" : "NO");
        Serial.printf("Drive Fault: %s\n", getDriveFault() ? "YES" : "NO");
    }
    Serial.println("==============================");
}

// Private methods

bool PicoBorgRev::rawWrite(uint8_t command, uint8_t* data, uint8_t dataLen) {
    if (!_wire) return false;

    _wire->beginTransmission(_address);
    _wire->write(command);

    for (uint8_t i = 0; i < dataLen; i++) {
        _wire->write(data[i]);
    }

    uint8_t error = _wire->endTransmission();
    if (error != 0) {
        Serial.printf("PBR: I2C write error %d for command 0x%02X\n", error, command);
        return false;
    }

    return true;
}

bool PicoBorgRev::rawRead(uint8_t command, uint8_t* buffer, uint8_t bufferLen, uint8_t retries) {
    if (!_wire) return false;

    while (retries > 0) {
        // Send command
        _wire->beginTransmission(_address);
        _wire->write(command);
        uint8_t error = _wire->endTransmission(false);  // false = repeated start

        if (error != 0) {
            Serial.printf("PBR: I2C read command error %d\n", error);
            retries--;
            delay(5);
            continue;
        }

        // Read response
        uint8_t bytesRead = _wire->requestFrom(_address, bufferLen);

        if (bytesRead != bufferLen) {
            Serial.printf("PBR: Expected %d bytes, got %d\n", bufferLen, bytesRead);
            retries--;
            delay(5);
            continue;
        }

        // Copy data to buffer
        for (uint8_t i = 0; i < bufferLen; i++) {
            buffer[i] = _wire->read();
        }

        // Verify command echo
        if (buffer[0] == command) {
            return true;
        }

        Serial.printf("PBR: Command mismatch: sent 0x%02X, got 0x%02X\n", command, buffer[0]);
        retries--;
        delay(5);
    }

    Serial.printf("PBR: Failed to read after retries\n");
    return false;
}

float PicoBorgRev::clampPower(float power) {
    if (power < -1.0f) return -1.0f;
    if (power > 1.0f) return 1.0f;
    return power;
}
