/**
 * WebServer.cpp
 *
 * Web server implementation with configuration UI
 */

#include "WebServer.h"
#include <ArduinoJson.h>

DiddyWebServer::DiddyWebServer(DriveController* drive, CameraComm* camera) {
    _drive = drive;
    _camera = camera;
    _server = nullptr;
    _running = false;
}

bool DiddyWebServer::begin(const char* ssid, const char* password) {
    Serial.println("WebServer: Starting WiFi AP...");

    // Start WiFi in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.printf("WebServer: AP started at %s\n", IP.toString().c_str());
    Serial.printf("WebServer: SSID: %s\n", ssid);
    Serial.printf("WebServer: Password: %s\n", password);

    // Create server
    _server = new AsyncWebServer(80);

    // Route handlers
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        this->handleRoot(req);
    });

    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        this->handleAPI(req);
    });

    _server->on("/api/camera/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        this->handleCameraAPI(req);
    });

    _server->on("/api/camera/record", HTTP_POST, [this](AsyncWebServerRequest* req) {
        bool start = req->hasParam("start");
        if (_camera) {
            if (start) {
                _camera->startRecording();
            } else {
                _camera->stopRecording();
            }
        }
        req->send(200, "text/plain", "OK");
    });

    _server->on("/api/camera/files", HTTP_GET, [this](AsyncWebServerRequest* req) {
        this->handleFileList(req);
    });

    _server->on("/api/camera/download", HTTP_GET, [this](AsyncWebServerRequest* req) {
        this->handleFileDownload(req);
    });

    _server->on("/api/camera/delete", HTTP_DELETE, [this](AsyncWebServerRequest* req) {
        this->handleFileDelete(req);
    });

    _server->on("/api/camera/setting", HTTP_POST, [this](AsyncWebServerRequest* req) {
        if (req->hasParam("key") && req->hasParam("value") && _camera) {
            String key = req->getParam("key")->value();
            String value = req->getParam("value")->value();
            _camera->setSetting(key.c_str(), value.c_str());
        }
        req->send(200, "text/plain", "OK");
    });

    _server->on("/api/config", HTTP_POST, [this](AsyncWebServerRequest* req) {
        if (req->hasParam("speed_limit")) {
            float limit = req->getParam("speed_limit")->value().toFloat();
            _drive->setSpeedLimit(limit);
        }
        if (req->hasParam("deadzone")) {
            float dz = req->getParam("deadzone")->value().toFloat();
            _drive->setDeadzone(dz);
        }
        req->send(200, "text/plain", "OK");
    });

    // Start server
    _server->begin();
    _running = true;

    Serial.println("WebServer: HTTP server started on port 80");
    Serial.printf("WebServer: Open http://%s in your browser\n", IP.toString().c_str());

    return true;
}

void DiddyWebServer::update() {
    // Server runs asynchronously, nothing needed here
}

String DiddyWebServer::getIPAddress() {
    return WiFi.softAPIP().toString();
}

void DiddyWebServer::handleRoot(AsyncWebServerRequest* request) {
    request->send(200, "text/html", generateHTML());
}

void DiddyWebServer::handleAPI(AsyncWebServerRequest* request) {
    request->send(200, "application/json", generateStatusJSON());
}

void DiddyWebServer::handleCameraAPI(AsyncWebServerRequest* request) {
    request->send(200, "application/json", generateCameraStatusJSON());
}

void DiddyWebServer::handleFileList(AsyncWebServerRequest* request) {
    if (_camera) {
        String files = _camera->getFileList();
        request->send(200, "application/json", files);
    } else {
        request->send(200, "application/json", "[]");
    }
}

void DiddyWebServer::handleFileDownload(AsyncWebServerRequest* request) {
    // File download via camera board's web server (proxy or redirect)
    if (_camera && request->hasParam("file")) {
        CameraStatus status = _camera->getStatus();
        String filename = request->getParam("file")->value();
        String url = "http://" + String(status.ipAddress) + ":81/download?file=" + filename;
        request->redirect(url);
    } else {
        request->send(404, "text/plain", "File not found");
    }
}

void DiddyWebServer::handleFileDelete(AsyncWebServerRequest* request) {
    if (_camera && request->hasParam("file")) {
        String filename = request->getParam("file")->value();
        bool ok = _camera->deleteFile(filename.c_str());
        request->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "ERROR");
    } else {
        request->send(400, "text/plain", "Missing filename");
    }
}

String DiddyWebServer::generateStatusJSON() {
    StaticJsonDocument<256> doc;

    doc["speed_limit"] = _drive->getSpeedLimit();
    doc["left_power"] = _drive->getLeftPower();
    doc["right_power"] = _drive->getRightPower();
    doc["camera_connected"] = _camera ? _camera->isConnected() : false;

    String json;
    serializeJson(doc, json);
    return json;
}

String DiddyWebServer::generateCameraStatusJSON() {
    if (!_camera) {
        return "{\"connected\":false}";
    }

    CameraStatus status = _camera->getStatus();
    StaticJsonDocument<512> doc;

    doc["connected"] = status.connected;
    doc["streaming"] = status.streaming;
    doc["recording"] = status.recording;
    doc["sd_total"] = status.sdTotal;
    doc["sd_used"] = status.sdUsed;
    doc["sd_free"] = status.sdFree;
    doc["file_count"] = status.fileCount;
    doc["ip_address"] = status.ipAddress;
    doc["stream_port"] = status.streamPort;

    if (status.connected) {
        doc["stream_url"] = "http://" + String(status.ipAddress) + ":" + String(status.streamPort) + "/stream";
    }

    String json;
    serializeJson(doc, json);
    return json;
}

String DiddyWebServer::generateHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DiddyBorg Control</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #1a1a1a;
            color: #fff;
            padding: 20px;
        }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { margin-bottom: 20px; color: #4CAF50; }
        h2 { margin: 20px 0 10px; color: #2196F3; font-size: 1.3em; }
        .section {
            background: #2a2a2a;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 15px;
        }
        .status-item {
            background: #333;
            padding: 15px;
            border-radius: 5px;
        }
        .status-label { color: #999; font-size: 0.9em; }
        .status-value { font-size: 1.5em; margin-top: 5px; }
        .btn {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1em;
            margin: 5px;
        }
        .btn:hover { background: #45a049; }
        .btn-danger { background: #f44336; }
        .btn-danger:hover { background: #da190b; }
        .btn-primary { background: #2196F3; }
        .btn-primary:hover { background: #0b7dda; }
        input[type="range"] {
            width: 100%;
            margin: 10px 0;
        }
        .slider-label {
            display: flex;
            justify-content: space-between;
            margin-top: 5px;
            font-size: 0.9em;
            color: #999;
        }
        .camera-offline {
            color: #f44336;
            font-style: italic;
        }
        .camera-online {
            color: #4CAF50;
        }
        #cameraStream {
            width: 100%;
            border-radius: 5px;
            margin-top: 10px;
            background: #000;
        }
        .file-list {
            max-height: 300px;
            overflow-y: auto;
            background: #333;
            padding: 10px;
            border-radius: 5px;
        }
        .file-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px;
            background: #444;
            margin-bottom: 5px;
            border-radius: 3px;
        }
        .file-item:hover { background: #555; }
        .setting-row {
            display: flex;
            align-items: center;
            margin-bottom: 10px;
        }
        .setting-row label {
            flex: 1;
            margin-right: 10px;
        }
        .setting-row input,
        .setting-row select {
            flex: 1;
            padding: 8px;
            border-radius: 3px;
            border: 1px solid #555;
            background: #333;
            color: #fff;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 DiddyBorg Control Panel</h1>

        <!-- Robot Status -->
        <div class="section">
            <h2>Robot Status</h2>
            <div class="status-grid">
                <div class="status-item">
                    <div class="status-label">Speed Limit</div>
                    <div class="status-value" id="speedLimit">--</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Left Motor</div>
                    <div class="status-value" id="leftPower">--</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Right Motor</div>
                    <div class="status-value" id="rightPower">--</div>
                </div>
            </div>
        </div>

        <!-- Drive Settings -->
        <div class="section">
            <h2>Drive Settings</h2>
            <div>
                <label>Speed Limit</label>
                <input type="range" id="speedSlider" min="0" max="100" value="70" oninput="updateSpeed(this.value)">
                <div class="slider-label">
                    <span>0%</span>
                    <span id="speedValue">70%</span>
                    <span>100%</span>
                </div>
            </div>
            <div style="margin-top:15px;">
                <label>Deadzone</label>
                <input type="range" id="deadzoneSlider" min="0" max="50" value="15" oninput="updateDeadzone(this.value)">
                <div class="slider-label">
                    <span>0%</span>
                    <span id="deadzoneValue">15%</span>
                    <span>50%</span>
                </div>
            </div>
        </div>

        <!-- Camera Section -->
        <div class="section" id="cameraSection">
            <h2>Camera <span id="cameraStatus" class="camera-offline">(Offline)</span></h2>

            <div id="cameraControls" style="display:none;">
                <div class="status-grid">
                    <div class="status-item">
                        <div class="status-label">SD Card</div>
                        <div class="status-value" id="sdSpace">--</div>
                    </div>
                    <div class="status-item">
                        <div class="status-label">Recordings</div>
                        <div class="status-value" id="fileCount">--</div>
                    </div>
                </div>

                <div style="margin: 15px 0;">
                    <button class="btn btn-primary" id="recordBtn" onclick="toggleRecording()">Start Recording</button>
                    <button class="btn" onclick="refreshFiles()">Refresh Files</button>
                    <a id="streamLink" class="btn btn-primary" href="#" target="_blank">Open Stream</a>
                </div>

                <h3 style="margin-top:20px;">Live Stream</h3>
                <img id="cameraStream" src="" alt="Camera stream will appear here" onerror="this.style.display='none'">

                <h3 style="margin-top:20px;">Recorded Files</h3>
                <div class="file-list" id="fileList">
                    <p style="color:#999;">No recordings yet</p>
                </div>

                <h3 style="margin-top:20px;">Camera Settings</h3>
                <div class="setting-row">
                    <label>Brightness</label>
                    <input type="range" min="-2" max="2" value="0" onchange="setCameraSetting('brightness', this.value)">
                </div>
                <div class="setting-row">
                    <label>Contrast</label>
                    <input type="range" min="-2" max="2" value="0" onchange="setCameraSetting('contrast', this.value)">
                </div>
                <div class="setting-row">
                    <label>Saturation</label>
                    <input type="range" min="-2" max="2" value="0" onchange="setCameraSetting('saturation', this.value)">
                </div>
                <div class="setting-row">
                    <label>Resolution</label>
                    <select onchange="setCameraSetting('framesize', this.value)">
                        <option value="8">SVGA (800x600)</option>
                        <option value="9">XGA (1024x768)</option>
                        <option value="7">VGA (640x480)</option>
                        <option value="6">CIF (400x296)</option>
                    </select>
                </div>
                <div class="setting-row">
                    <label>Quality (lower=better)</label>
                    <input type="range" min="10" max="63" value="12" onchange="setCameraSetting('quality', this.value)">
                </div>
            </div>
        </div>
    </div>

    <script>
        let recording = false;

        function updateStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('speedLimit').textContent = Math.round(data.speed_limit * 100) + '%';
                    document.getElementById('leftPower').textContent = Math.round(data.left_power * 100) + '%';
                    document.getElementById('rightPower').textContent = Math.round(data.right_power * 100) + '%';
                });

            fetch('/api/camera/status')
                .then(r => r.json())
                .then(data => {
                    if (data.connected) {
                        document.getElementById('cameraStatus').textContent = '(Online)';
                        document.getElementById('cameraStatus').className = 'camera-online';
                        document.getElementById('cameraControls').style.display = 'block';

                        document.getElementById('sdSpace').textContent =
                            data.sd_used + '/' + data.sd_total + ' MB';
                        document.getElementById('fileCount').textContent = data.file_count;

                        if (data.stream_url) {
                            document.getElementById('streamLink').href = data.stream_url;
                            document.getElementById('cameraStream').src = data.stream_url;
                            document.getElementById('cameraStream').style.display = 'block';
                        }

                        if (data.recording) {
                            recording = true;
                            document.getElementById('recordBtn').textContent = 'Stop Recording';
                            document.getElementById('recordBtn').className = 'btn btn-danger';
                        } else {
                            recording = false;
                            document.getElementById('recordBtn').textContent = 'Start Recording';
                            document.getElementById('recordBtn').className = 'btn btn-primary';
                        }
                    } else {
                        document.getElementById('cameraStatus').textContent = '(Offline)';
                        document.getElementById('cameraStatus').className = 'camera-offline';
                        document.getElementById('cameraControls').style.display = 'none';
                    }
                });
        }

        function updateSpeed(value) {
            document.getElementById('speedValue').textContent = value + '%';
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'speed_limit=' + (value / 100)
            });
        }

        function updateDeadzone(value) {
            document.getElementById('deadzoneValue').textContent = value + '%';
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'deadzone=' + (value / 100)
            });
        }

        function toggleRecording() {
            fetch('/api/camera/record', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: recording ? '' : 'start=1'
            }).then(() => updateStatus());
        }

        function setCameraSetting(key, value) {
            fetch('/api/camera/setting', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'key=' + key + '&value=' + value
            });
        }

        function refreshFiles() {
            fetch('/api/camera/files')
                .then(r => r.json())
                .then(files => {
                    let html = '';
                    if (files.length === 0) {
                        html = '<p style="color:#999;">No recordings yet</p>';
                    } else {
                        files.forEach(file => {
                            html += `<div class="file-item">
                                <span>${file.name} (${(file.size/1024/1024).toFixed(1)}MB)</span>
                                <div>
                                    <a href="/api/camera/download?file=${file.name}" class="btn" download>Download</a>
                                    <button class="btn btn-danger" onclick="deleteFile('${file.name}')">Delete</button>
                                </div>
                            </div>`;
                        });
                    }
                    document.getElementById('fileList').innerHTML = html;
                });
        }

        function deleteFile(filename) {
            if (confirm('Delete ' + filename + '?')) {
                fetch('/api/camera/delete?file=' + filename, {method: 'DELETE'})
                    .then(() => refreshFiles());
            }
        }

        // Update every 2 seconds
        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

    return html;
}
