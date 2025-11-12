/**
 * DebugLog.cpp
 *
 * Debug log implementation
 */

#include "DebugLog.h"
#include <stdarg.h>

// Global instance
DebugLog debugLog;

DebugLog::DebugLog() {
    _head = 0;
    _count = 0;
}

void DebugLog::log(const char* message) {
    String timestamp = getTimestamp();
    _entries[_head] = timestamp + " " + String(message);

    _head = (_head + 1) % DEBUG_LOG_SIZE;
    if (_count < DEBUG_LOG_SIZE) {
        _count++;
    }
}

void DebugLog::logf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    log(buffer);
}

String DebugLog::getAll() const {
    String result = "";

    if (_count == 0) {
        return "No log entries";
    }

    // Calculate start position (oldest entry)
    int start = (_count < DEBUG_LOG_SIZE) ? 0 : _head;

    // Read entries in chronological order
    for (int i = 0; i < _count; i++) {
        int index = (start + i) % DEBUG_LOG_SIZE;
        result += _entries[index] + "\n";
    }

    return result;
}

void DebugLog::clear() {
    _head = 0;
    _count = 0;
}

String DebugLog::getTimestamp() const {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "[%02lu:%02lu:%02lu]", hours, minutes, seconds);
    return String(buffer);
}
