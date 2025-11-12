/**
 * WebServer.h
 *
 * Web configuration interface for DiddyBorg
 * Provides control over motor settings and camera (if connected)
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "CameraComm.h"
#include "DriveController.h"

class DiddyWebServer {
public:
    DiddyWebServer(DriveController* drive, CameraComm* camera);

    // Start web server (also starts WiFi AP)
    bool begin(const char* ssid = "DiddyBorg", const char* password = "diddyborg123");

    // Update loop
    void update();

    // Get server status
    bool isRunning() { return _running; }
    String getIPAddress();

private:
    AsyncWebServer* _server;
    DriveController* _drive;
    CameraComm* _camera;
    Preferences _prefs;
    bool _running;

    // Page handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleAPI(AsyncWebServerRequest* request);
    void handleCameraAPI(AsyncWebServerRequest* request);
    void handleFileList(AsyncWebServerRequest* request);
    void handleFileDownload(AsyncWebServerRequest* request);
    void handleFileDelete(AsyncWebServerRequest* request);

    // HTML generation
    String generateHTML();
    String generateStatusJSON();
    String generateCameraStatusJSON();

    // Configuration
    void loadPreferences();
    void savePreferences();
};

#endif // WEB_SERVER_H
