/**
 * CameraComm.cpp
 *
 * UART communication implementation
 */

#include "CameraComm.h"
#include <ArduinoJson.h>

CameraComm::CameraComm() {
    _serial = nullptr;
    memset(&_status, 0, sizeof(_status));
    _status.streamPort = 81;  // Default
}

bool CameraComm::begin() {
    _serial = &Serial1;
    _serial->begin(CAMERA_UART_BAUD, SERIAL_8N1, CAMERA_UART_RX, CAMERA_UART_TX);

    Serial.println("CameraComm: UART initialized");

    // Wait a moment for camera board to boot
    delay(100);

    // Test connection
    return ping();
}

bool CameraComm::isConnected() {
    // Consider connected if seen within last 5 seconds
    return (millis() - _status.lastSeen) < 5000;
}

bool CameraComm::ping() {
    String response = sendCommand(CMD_PING, 500);
    if (response.startsWith(RESP_PONG)) {
        _status.connected = true;
        _status.lastSeen = millis();
        Serial.println("CameraComm: Ping successful");
        return true;
    }
    _status.connected = false;
    return false;
}

bool CameraComm::startRecording() {
    String response = sendCommand(CMD_START_REC);
    return response.startsWith(RESP_OK);
}

bool CameraComm::stopRecording() {
    String response = sendCommand(CMD_STOP_REC);
    return response.startsWith(RESP_OK);
}

bool CameraComm::setSetting(const char* key, const char* value) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "%s%s=%s", CMD_SET_SETTING, key, value);
    String response = sendCommand(cmd);
    return response.startsWith(RESP_OK);
}

String CameraComm::getSetting(const char* key) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s%s", CMD_GET_SETTING, key);
    String response = sendCommand(cmd);

    if (response.startsWith(RESP_SETTING)) {
        return response.substring(strlen(RESP_SETTING));
    }
    return "";
}

bool CameraComm::deleteFile(const char* filename) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "%s%s", CMD_DELETE_FILE, filename);
    String response = sendCommand(cmd);
    return response.startsWith(RESP_OK);
}

CameraStatus CameraComm::getStatus() {
    String response = sendCommand(CMD_GET_STATUS, 2000);

    if (response.startsWith(RESP_STATUS)) {
        String json = response.substring(strlen(RESP_STATUS));
        parseStatus(json.c_str());
    }

    return _status;
}

String CameraComm::getFileList() {
    String response = sendCommand(CMD_GET_FILES, 3000);

    if (response.startsWith(RESP_FILES)) {
        return response.substring(strlen(RESP_FILES));
    }
    return "[]";
}

void CameraComm::update() {
    processIncoming();

    // Periodic ping to maintain connection status
    static unsigned long lastPing = 0;
    if (millis() - lastPing > 2000) {
        lastPing = millis();
        ping();
    }
}

// Private methods

String CameraComm::sendCommand(const char* command, uint32_t timeout) {
    if (!_serial) return "";

    // Clear any pending data
    while (_serial->available()) {
        _serial->read();
    }

    // Send command
    _serial->println(command);
    _serial->flush();

    // Wait for response
    String response = "";
    unsigned long start = millis();

    while (millis() - start < timeout) {
        if (_serial->available()) {
            char c = _serial->read();
            if (c == '\n') {
                break;  // Complete line received
            }
            response += c;
        }
        delay(1);
    }

    return response;
}

void CameraComm::parseStatus(const char* json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.printf("CameraComm: JSON parse error: %s\n", error.c_str());
        return;
    }

    _status.connected = true;
    _status.streaming = doc["streaming"] | false;
    _status.recording = doc["recording"] | false;
    _status.sdTotal = doc["sd_total"] | 0;
    _status.sdUsed = doc["sd_used"] | 0;
    _status.sdFree = doc["sd_free"] | 0;
    _status.fileCount = doc["file_count"] | 0;
    _status.streamPort = doc["stream_port"] | 81;
    _status.lastSeen = millis();

    const char* ip = doc["ip"] | "0.0.0.0";
    strncpy(_status.ipAddress, ip, sizeof(_status.ipAddress) - 1);
}

void CameraComm::processIncoming() {
    if (!_serial) return;

    while (_serial->available()) {
        char c = _serial->read();
        if (c == '\n') {
            // Process complete line
            // (For future: handle unsolicited messages from camera)
            _receiveBuffer = "";
        } else {
            _receiveBuffer += c;
        }
    }
}
