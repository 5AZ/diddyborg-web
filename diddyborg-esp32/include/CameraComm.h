/**
 * CameraComm.h
 *
 * UART communication protocol between motor controller and camera board
 */

#ifndef CAMERA_COMM_H
#define CAMERA_COMM_H

#include <Arduino.h>
#include <HardwareSerial.h>

// UART pins for camera communication
#define CAMERA_UART_TX      17
#define CAMERA_UART_RX      18
#define CAMERA_UART_BAUD    115200

// Commands sent TO camera board
#define CMD_PING            "PING"
#define CMD_START_REC       "REC_START"
#define CMD_STOP_REC        "REC_STOP"
#define CMD_GET_STATUS      "STATUS"
#define CMD_GET_FILES       "FILES"
#define CMD_DELETE_FILE     "DELETE:"    // Followed by filename
#define CMD_SET_SETTING     "SET:"       // Followed by key=value
#define CMD_GET_SETTING     "GET:"       // Followed by key

// Responses FROM camera board
#define RESP_PONG           "PONG"
#define RESP_OK             "OK"
#define RESP_ERROR          "ERROR"
#define RESP_STATUS         "STATUS:"    // Followed by JSON
#define RESP_FILES          "FILES:"     // Followed by JSON
#define RESP_SETTING        "SETTING:"   // Followed by key=value

// Camera board status structure
struct CameraStatus {
    bool connected;
    bool streaming;
    bool recording;
    uint32_t sdTotal;       // MB
    uint32_t sdUsed;        // MB
    uint32_t sdFree;        // MB
    uint16_t fileCount;
    char ipAddress[16];
    uint16_t streamPort;
    unsigned long lastSeen;
};

class CameraComm {
public:
    CameraComm();

    // Initialize UART communication
    bool begin();

    // Check if camera board is connected
    bool isConnected();

    // Send commands
    bool ping();
    bool startRecording();
    bool stopRecording();
    bool setSetting(const char* key, const char* value);
    String getSetting(const char* key);
    bool deleteFile(const char* filename);

    // Get status
    CameraStatus getStatus();
    String getFileList();

    // Update loop - call frequently
    void update();

private:
    HardwareSerial* _serial;
    CameraStatus _status;
    String _receiveBuffer;

    // Send command and wait for response
    String sendCommand(const char* command, uint32_t timeout = 1000);

    // Parse status JSON
    void parseStatus(const char* json);

    // Check for incoming messages
    void processIncoming();
};

#endif // CAMERA_COMM_H
