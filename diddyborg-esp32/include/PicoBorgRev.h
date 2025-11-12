/**
 * PicoBorgRev.h
 *
 * ESP32 Arduino library for PicoBorg Reverse motor controller
 * Ported from PiBorg's Python library
 *
 * Controls two DC motors via I2C interface
 * Motor power range: -1.0 (full reverse) to +1.0 (full forward)
 */

#ifndef PICOBORG_REV_H
#define PICOBORG_REV_H

#include <Arduino.h>
#include <Wire.h>

// I2C Constants
#define PBR_DEFAULT_ADDRESS     0x44
#define PBR_I2C_ID              0x15
#define PBR_I2C_MAX_LEN         4
#define PBR_PWM_MAX             255

// Command codes
#define PBR_CMD_SET_LED         1
#define PBR_CMD_GET_LED         2
#define PBR_CMD_SET_A_FWD       3
#define PBR_CMD_SET_A_REV       4
#define PBR_CMD_GET_A           5
#define PBR_CMD_SET_B_FWD       6
#define PBR_CMD_SET_B_REV       7
#define PBR_CMD_GET_B           8
#define PBR_CMD_ALL_OFF         9
#define PBR_CMD_RESET_EPO       10
#define PBR_CMD_GET_EPO         11
#define PBR_CMD_SET_EPO_IGNORE  12
#define PBR_CMD_GET_EPO_IGNORE  13
#define PBR_CMD_GET_DRIVE_FAULT 14
#define PBR_CMD_SET_ALL_FWD     15
#define PBR_CMD_SET_ALL_REV     16
#define PBR_CMD_SET_FAILSAFE    17
#define PBR_CMD_GET_FAILSAFE    18
#define PBR_CMD_GET_ID          0x99

// Command values
#define PBR_VALUE_FWD           1
#define PBR_VALUE_REV           2
#define PBR_VALUE_ON            1
#define PBR_VALUE_OFF           0

class PicoBorgRev {
public:
    PicoBorgRev(uint8_t address = PBR_DEFAULT_ADDRESS);

    // Initialization
    bool begin(TwoWire &wirePort = Wire);
    bool isConnected();

    // Motor control (Motor 1 = typically RIGHT side, Motor 2 = typically LEFT side)
    void setMotor1(float power);  // -1.0 to +1.0
    void setMotor2(float power);  // -1.0 to +1.0
    void setMotors(float power);  // Set both motors to same power
    void motorsOff();              // Emergency stop

    // LED control
    void setLed(bool state);
    bool getLed();

    // Safety features
    void resetEpo();
    bool getEpo();
    void setEpoIgnore(bool state);
    void setCommsFailsafe(bool state);
    bool getDriveFault();

    // Diagnostic
    void printStatus();

private:
    TwoWire* _wire;
    uint8_t _address;
    bool _foundChip;

    // Low-level I2C communication
    bool rawWrite(uint8_t command, uint8_t* data, uint8_t dataLen);
    bool rawRead(uint8_t command, uint8_t* buffer, uint8_t bufferLen, uint8_t retries = 3);

    // Helper to clamp power values
    float clampPower(float power);
};

#endif // PICOBORG_REV_H
