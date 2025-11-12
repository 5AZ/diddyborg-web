/**
 * FlyskyInput.cpp
 *
 * Flysky RC receiver input implementation
 */

#include "FlyskyInput.h"

FlyskyInput* FlyskyInput::_instance = nullptr;

FlyskyInput::FlyskyInput() {
    _ppmMode = false;
    _ppmPin = 0;
    _numChannels = 0;
    _deadzone = FLYSKY_PULSE_DEADZONE;
    _ppmChannelIndex = 0;
    _ppmLastPulse = 0;

    for (uint8_t i = 0; i < FLYSKY_MAX_CHANNELS; i++) {
        _channelValues[i] = FLYSKY_PULSE_MID;
        _ppmChannels[i] = FLYSKY_PULSE_MID;
        _lastUpdate[i] = 0;
        _channelReverse[i] = false;
        _channelPins[i] = 0;
    }

    _instance = this;
}

bool FlyskyInput::beginPPM(uint8_t ppmPin) {
    _ppmMode = true;
    _ppmPin = ppmPin;
    _numChannels = FLYSKY_MAX_CHANNELS;

    pinMode(_ppmPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_ppmPin), ppmISR, RISING);

    Serial.printf("Flysky: PPM mode initialized on pin %d\n", _ppmPin);
    return true;
}

bool FlyskyInput::beginPWM(uint8_t* channelPins, uint8_t numChannels) {
    _ppmMode = false;
    _numChannels = min(numChannels, (uint8_t)FLYSKY_MAX_CHANNELS);

    for (uint8_t i = 0; i < _numChannels; i++) {
        _channelPins[i] = channelPins[i];
        pinMode(_channelPins[i], INPUT);
    }

    Serial.printf("Flysky: PWM mode initialized with %d channels\n", _numChannels);
    return true;
}

bool FlyskyInput::isConnected() {
    unsigned long now = millis();

    // Check if we've received recent updates on at least one channel
    for (uint8_t i = 0; i < _numChannels; i++) {
        if ((now - _lastUpdate[i]) < FLYSKY_SIGNAL_TIMEOUT) {
            return true;
        }
    }

    return false;
}

uint16_t FlyskyInput::getRawChannel(uint8_t channel) {
    if (channel >= _numChannels) return FLYSKY_PULSE_MID;
    return _channelValues[channel];
}

float FlyskyInput::getChannel(uint8_t channel) {
    if (channel >= _numChannels) return 0.0f;

    uint16_t pulse = _channelValues[channel];
    float value = pulseToFloat(pulse);

    if (_channelReverse[channel]) {
        value = -value;
    }

    return value;
}

float FlyskyInput::getThrottle() {
    return getChannel(2);  // Channel 3 (index 2) - left stick Y
}

float FlyskyInput::getSteering() {
    return getChannel(0);  // Channel 1 (index 0) - right stick X
}

float FlyskyInput::getLeftStick() {
    return getChannel(3);  // Channel 4 (index 3) - right stick Y
}

float FlyskyInput::getRightStick() {
    return getChannel(1);  // Channel 2 (index 1) - left stick X
}

void FlyskyInput::setChannelReverse(uint8_t channel, bool reverse) {
    if (channel < FLYSKY_MAX_CHANNELS) {
        _channelReverse[channel] = reverse;
    }
}

void FlyskyInput::update() {
    unsigned long now = millis();

    if (_ppmMode) {
        // Copy PPM values from interrupt handler
        noInterrupts();
        for (uint8_t i = 0; i < _numChannels; i++) {
            _channelValues[i] = _ppmChannels[i];
        }
        interrupts();

        // Update timestamps for all channels
        for (uint8_t i = 0; i < _numChannels; i++) {
            _lastUpdate[i] = now;
        }
    } else {
        // PWM mode - measure each channel
        for (uint8_t i = 0; i < _numChannels; i++) {
            uint16_t pulse = measurePulse(_channelPins[i]);
            if (pulse > 0) {  // Valid pulse received
                _channelValues[i] = pulse;
                _lastUpdate[i] = now;
            }
        }
    }
}

// Private methods

void IRAM_ATTR FlyskyInput::ppmISR() {
    if (!_instance) return;

    unsigned long now = micros();
    unsigned long pulseWidth = now - _instance->_ppmLastPulse;
    _instance->_ppmLastPulse = now;

    // PPM frame sync pulse is typically > 3000us
    if (pulseWidth > 3000) {
        _instance->_ppmChannelIndex = 0;
    } else if (pulseWidth >= FLYSKY_PULSE_MIN && pulseWidth <= FLYSKY_PULSE_MAX) {
        // Valid channel pulse
        if (_instance->_ppmChannelIndex < FLYSKY_MAX_CHANNELS) {
            _instance->_ppmChannels[_instance->_ppmChannelIndex] = pulseWidth;
            _instance->_ppmChannelIndex++;
        }
    }
}

uint16_t FlyskyInput::measurePulse(uint8_t pin) {
    // Wait for pulse to start (with timeout)
    unsigned long timeout = micros() + 25000;  // 25ms timeout
    while (digitalRead(pin) == LOW && micros() < timeout);

    if (micros() >= timeout) return 0;

    // Measure pulse width
    unsigned long start = micros();
    timeout = start + 3000;  // 3ms max pulse width

    while (digitalRead(pin) == HIGH && micros() < timeout);

    if (micros() >= timeout) return 0;

    uint16_t pulse = micros() - start;

    // Validate pulse width
    if (pulse < FLYSKY_PULSE_MIN || pulse > FLYSKY_PULSE_MAX) {
        return 0;
    }

    return pulse;
}

float FlyskyInput::pulseToFloat(uint16_t pulse) {
    // Apply deadzone around center
    int16_t centered = pulse - FLYSKY_PULSE_MID;

    if (abs(centered) < _deadzone) {
        return 0.0f;
    }

    // Scale to -1.0 to 1.0
    if (centered > 0) {
        return (float)(centered - _deadzone) / (float)(FLYSKY_PULSE_MAX - FLYSKY_PULSE_MID - _deadzone);
    } else {
        return (float)(centered + _deadzone) / (float)(FLYSKY_PULSE_MID - FLYSKY_PULSE_MIN - _deadzone);
    }
}
