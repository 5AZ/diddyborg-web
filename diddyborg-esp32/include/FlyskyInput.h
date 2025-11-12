/**
 * FlyskyInput.h
 *
 * Flysky FS-i6 RC receiver input handler
 * Supports both PPM (single pin) and PWM (multiple pins) modes
 */

#ifndef FLYSKY_INPUT_H
#define FLYSKY_INPUT_H

#include <Arduino.h>

#define FLYSKY_MAX_CHANNELS 6
#define FLYSKY_PULSE_MIN 1000
#define FLYSKY_PULSE_MID 1500
#define FLYSKY_PULSE_MAX 2000
#define FLYSKY_PULSE_DEADZONE 50
#define FLYSKY_SIGNAL_TIMEOUT 100  // ms

class FlyskyInput {
public:
    FlyskyInput();

    // PPM mode (single pin for all channels)
    bool beginPPM(uint8_t ppmPin);

    // PWM mode (individual pins for each channel)
    bool beginPWM(uint8_t* channelPins, uint8_t numChannels);

    // Check if receiver is connected and sending valid signals
    bool isConnected();

    // Get channel value as raw pulse width (1000-2000 microseconds)
    uint16_t getRawChannel(uint8_t channel);

    // Get channel value normalized to -1.0 to +1.0 range
    float getChannel(uint8_t channel);

    // Get specific controls for tank/arcade driving
    float getThrottle();  // Typically channel 3 (left stick Y)
    float getSteering();  // Typically channel 1 (right stick X)
    float getLeftStick(); // Channel 4 (right stick Y) for tank mode
    float getRightStick(); // Channel 2 (left stick X) for tank mode

    // Configuration
    void setChannelReverse(uint8_t channel, bool reverse);
    void setDeadzone(uint16_t deadzone) { _deadzone = deadzone; }

    // Update loop (call frequently)
    void update();

private:
    bool _ppmMode;
    uint8_t _ppmPin;
    uint8_t _numChannels;
    uint8_t _channelPins[FLYSKY_MAX_CHANNELS];

    uint16_t _channelValues[FLYSKY_MAX_CHANNELS];
    unsigned long _lastUpdate[FLYSKY_MAX_CHANNELS];
    bool _channelReverse[FLYSKY_MAX_CHANNELS];
    uint16_t _deadzone;

    // PPM decoding state
    volatile uint16_t _ppmChannels[FLYSKY_MAX_CHANNELS];
    volatile uint8_t _ppmChannelIndex;
    volatile unsigned long _ppmLastPulse;

    // ISR handler for PPM
    static void IRAM_ATTR ppmISR();
    static FlyskyInput* _instance;  // For ISR callback

    // PWM pulse measurement
    uint16_t measurePulse(uint8_t pin);

    // Helper to normalize pulse to -1.0 to 1.0
    float pulseToFloat(uint16_t pulse);
};

#endif // FLYSKY_INPUT_H
