/**
 * WebAuth.h
 *
 * Simple PIN-based authentication for web UI
 */

#ifndef WEB_AUTH_H
#define WEB_AUTH_H

#include <Arduino.h>
#include <Preferences.h>

class WebAuth {
public:
    WebAuth();

    // Initialize with default PIN
    void begin(const char* defaultPin);

    // Check if provided PIN is correct
    bool verifyPin(const char* pin);

    // Change PIN (returns true if successful)
    bool changePin(const char* oldPin, const char* newPin);

    // Force set PIN (used when camera board requests sync with shared secret)
    void forceSetPin(const char* newPin);

    // Get current PIN (for syncing to camera)
    String getCurrentPin();

    // Generate session token
    String generateSessionToken();

    // Verify session token
    bool verifySessionToken(const char* token);

    // Invalidate session token (logout)
    void invalidateSession(const char* token);

    // Clean up expired sessions
    void cleanupSessions();

private:
    Preferences _prefs;
    String _currentPin;

    // Simple session storage (token -> expiry time)
    struct Session {
        String token;
        unsigned long expiry;
    };

    static const int MAX_SESSIONS = 5;
    Session _sessions[MAX_SESSIONS];

    // Generate random token
    String generateRandomToken();
};

#endif // WEB_AUTH_H
