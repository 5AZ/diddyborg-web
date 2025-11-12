/**
 * DebugLog.h
 *
 * Circular buffer debug log for web UI access
 * Stores last N lines of meaningful events only
 */

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Arduino.h>

#define DEBUG_LOG_SIZE 250  // Store last 250 log lines

class DebugLog {
public:
    DebugLog();

    // Add log entry (automatically adds timestamp)
    void log(const char* message);
    void logf(const char* format, ...);  // Printf-style formatting

    // Get all log entries as newline-separated string
    String getAll() const;

    // Clear all entries
    void clear();

    // Get entry count
    int getCount() const { return _count; }

private:
    String _entries[DEBUG_LOG_SIZE];
    int _head;   // Next write position
    int _count;  // Number of valid entries

    String getTimestamp() const;
};

// Global debug log instance (declared in DebugLog.cpp)
extern DebugLog debugLog;

#endif // DEBUG_LOG_H
