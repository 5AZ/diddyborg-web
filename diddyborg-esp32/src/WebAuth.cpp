/**
 * WebAuth.cpp
 *
 * PIN-based authentication implementation
 */

#include "WebAuth.h"
#include "Config.h"
#include "DebugLog.h"

WebAuth::WebAuth() {
    _currentPin = "";
    for (int i = 0; i < MAX_SESSIONS; i++) {
        _sessions[i].token = "";
        _sessions[i].expiry = 0;
    }
}

void WebAuth::begin(const char* defaultPin) {
    _prefs.begin("webauth", false);

    // Initialize random seed for session token generation
    // Use unconnected analog pin + microseconds for entropy
    randomSeed(analogRead(0) ^ micros());

    // Load saved PIN or use default
    _currentPin = _prefs.getString("pin", defaultPin);

    // Validate PIN format (6-8 digits)
    if (_currentPin.length() < 6 || _currentPin.length() > 8) {
        Serial.println("WebAuth: Invalid saved PIN, using default");
        _currentPin = String(defaultPin);
        _prefs.putString("pin", _currentPin);
    }

    Serial.printf("WebAuth: Initialized with PIN: %s\n", _currentPin.c_str());
}

bool WebAuth::verifyPin(const char* pin) {
    bool valid = (_currentPin == String(pin));
    if (valid) {
        debugLog.log("AUTH: Login successful");
    } else {
        debugLog.log("AUTH: Login failed - invalid PIN");
    }
    return valid;
}

bool WebAuth::changePin(const char* oldPin, const char* newPin) {
    // Verify old PIN
    if (!verifyPin(oldPin)) {
        Serial.println("WebAuth: Old PIN incorrect");
        return false;
    }

    // Validate new PIN (6-8 digits)
    String newPinStr = String(newPin);
    if (newPinStr.length() < 6 || newPinStr.length() > 8) {
        Serial.println("WebAuth: New PIN must be 6-8 digits");
        return false;
    }

    // Check if all digits
    for (size_t i = 0; i < newPinStr.length(); i++) {
        if (!isDigit(newPinStr[i])) {
            Serial.println("WebAuth: PIN must contain only digits");
            return false;
        }
    }

    // Update PIN
    _currentPin = newPinStr;
    _prefs.putString("pin", _currentPin);

    // Invalidate all sessions (force re-login)
    for (int i = 0; i < MAX_SESSIONS; i++) {
        _sessions[i].token = "";
        _sessions[i].expiry = 0;
    }

    Serial.printf("WebAuth: PIN changed to: %s\n", _currentPin.c_str());
    debugLog.log("AUTH: PIN changed successfully - all sessions invalidated");
    return true;
}

void WebAuth::forceSetPin(const char* newPin) {
    // Used when camera board sends PIN sync with valid shared secret
    String newPinStr = String(newPin);

    // Basic validation
    if (newPinStr.length() >= 6 && newPinStr.length() <= 8) {
        _currentPin = newPinStr;
        _prefs.putString("pin", _currentPin);
        Serial.printf("WebAuth: PIN force-updated to: %s\n", _currentPin.c_str());
    }
}

String WebAuth::getCurrentPin() {
    return _currentPin;
}

String WebAuth::generateSessionToken() {
    String token = generateRandomToken();

    // Find empty slot or oldest session
    int oldestIdx = 0;
    unsigned long oldestTime = ULONG_MAX;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (_sessions[i].token == "") {
            oldestIdx = i;
            break;
        }
        if (_sessions[i].expiry < oldestTime) {
            oldestTime = _sessions[i].expiry;
            oldestIdx = i;
        }
    }

    // Store session
    _sessions[oldestIdx].token = token;
    _sessions[oldestIdx].expiry = millis() + (SESSION_TIMEOUT_SECONDS * 1000);

    Serial.printf("WebAuth: Generated session token: %s\n", token.c_str());
    return token;
}

bool WebAuth::verifySessionToken(const char* token) {
    if (!token || strlen(token) == 0) {
        return false;
    }

    unsigned long now = millis();
    String tokenStr = String(token);

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (_sessions[i].token == tokenStr) {
            if (_sessions[i].expiry > now) {
                // Valid and not expired
                return true;
            } else {
                // Expired
                _sessions[i].token = "";
                _sessions[i].expiry = 0;
                return false;
            }
        }
    }

    return false;
}

void WebAuth::invalidateSession(const char* token) {
    String tokenStr = String(token);

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (_sessions[i].token == tokenStr) {
            _sessions[i].token = "";
            _sessions[i].expiry = 0;
            Serial.printf("WebAuth: Session invalidated: %s\n", token);
            return;
        }
    }
}

void WebAuth::cleanupSessions() {
    unsigned long now = millis();
    int cleaned = 0;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (_sessions[i].token != "" && _sessions[i].expiry < now) {
            _sessions[i].token = "";
            _sessions[i].expiry = 0;
            cleaned++;
        }
    }

    if (cleaned > 0) {
        Serial.printf("WebAuth: Cleaned up %d expired sessions\n", cleaned);
    }
}

String WebAuth::generateRandomToken() {
    // Generate 32-character random token
    String token = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = 0; i < 32; i++) {
        token += charset[random(0, sizeof(charset) - 1)];
    }

    return token;
}
