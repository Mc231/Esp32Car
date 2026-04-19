#include "RoverWebServer.h"
#include "Html/control_html.h"
#include <Update.h>

RoverWebServer::RoverWebServer(RoverController& carController, RoverApplicationConfig config, SystemMonitor&)
  : carController(carController), config(config), systemMonitor(systemMonitor), server(config.webServerPort)
{}

bool RoverWebServer::authorized() {
  // Auth disabled when both fields empty.
  if (!config.adminUser || !config.adminPassword || config.adminUser[0] == '\0') {
    return true;
  }
  if (server.authenticate(config.adminUser, config.adminPassword)) {
    return true;
  }
  server.requestAuthentication();
  return false;
}
 
void RoverWebServer::begin() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); }); 
  server.on("/reboot", HTTP_POST, [this]() { handleReboot(); });
  server.on("/wifi", HTTP_GET, [this]() { handleGetWiFi(); });
  server.on("/wifi/forget", HTTP_POST, [this]() { handleWiFiForget(); });
  server.on("/motor", HTTP_PUT, [this]() { handleSetMotor(); }); 
  server.on("/motor", HTTP_GET, [this]() { handleMotorState(); }); 
  server.on("/ultrasonic", HTTP_GET, [this]() { handleGetUltrasonic(); });
  server.on("/motorPWM", HTTP_PUT, [this]() { handleSetMotorPWM(); });
  server.on("/camera", HTTP_PUT, [this]() { handleCamera(); });
  server.on("/system", HTTP_GET, [this]() { handleSystem(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); } );
  server.on("/config", HTTP_GET, [this]() { handleConfig();} );
  server.on("/ota", HTTP_GET, [this]() { handleOtaPage(); });
  server.on("/ota/upload", HTTP_POST,
    [this]() { handleOtaUploadFinish(); },
    [this]() { handleOtaUpload(); }
  );
  server.begin(config.webServerPort);
}

static const char OTA_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rover OTA</title>
<style>
:root{--bg:#0b1020;--card:#1a2240;--border:#2a345a;--text:#e7ebf5;--dim:#8a93b3;--accent:#5b8cff;}
*{box-sizing:border-box}body{margin:0;font-family:-apple-system,Segoe UI,Roboto,sans-serif;color:var(--text);background:var(--bg);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:22px;max-width:420px;width:100%;box-shadow:0 12px 40px rgba(0,0,0,.4)}
h1{margin:0 0 6px;font-size:20px}p{margin:0 0 18px;color:var(--dim);font-size:13px}
input[type=file]{width:100%;padding:10px;background:#131a30;border:1px dashed var(--border);border-radius:10px;color:var(--text);margin-bottom:12px;font-size:13px}
button{width:100%;padding:12px;border:none;border-radius:10px;background:linear-gradient(135deg,var(--accent),#8a5bff);color:#fff;font-weight:600;cursor:pointer;font-size:14px}
button:disabled{opacity:.5;cursor:not-allowed}
.bar{margin-top:14px;height:8px;background:#131a30;border-radius:999px;overflow:hidden;display:none}
.bar.show{display:block}.fill{height:100%;width:0;background:linear-gradient(90deg,var(--accent),#8a5bff);transition:width .15s}
.msg{margin-top:12px;font-size:13px;text-align:center}.ok{color:#3ddc84}.err{color:#ff6b6b}
a{color:var(--accent);text-decoration:none;font-size:13px;display:inline-block;margin-top:14px}
</style></head><body>
<div class="card">
<h1>Firmware Update</h1>
<p>Upload a compiled <code>firmware.bin</code> to flash the rover over the air.</p>
<form id="f"><input type="file" id="file" accept=".bin" required>
<button id="b" type="submit">Upload &amp; Flash</button>
<div class="bar" id="bar"><div class="fill" id="fill"></div></div>
<div class="msg" id="msg"></div></form>
<a href="/">&larr; Back to control</a>
</div>
<script>
const f=document.getElementById('f'),file=document.getElementById('file'),b=document.getElementById('b'),
bar=document.getElementById('bar'),fill=document.getElementById('fill'),msg=document.getElementById('msg');
f.addEventListener('submit',e=>{e.preventDefault();if(!file.files[0])return;
const fd=new FormData();fd.append('update',file.files[0]);
const xhr=new XMLHttpRequest();xhr.open('POST','/ota/upload');
xhr.upload.onprogress=ev=>{if(ev.lengthComputable){bar.classList.add('show');
fill.style.width=(ev.loaded/ev.total*100)+'%';msg.textContent=Math.round(ev.loaded/ev.total*100)+'%';}};
xhr.onload=()=>{if(xhr.status===200){msg.className='msg ok';msg.textContent='Upload complete. Rebooting...';}
else{msg.className='msg err';msg.textContent='Failed: '+xhr.responseText;b.disabled=false;}};
xhr.onerror=()=>{msg.className='msg err';msg.textContent='Network error';b.disabled=false;};
b.disabled=true;msg.className='msg';msg.textContent='Uploading...';xhr.send(fd);});
</script></body></html>
)rawliteral";

void RoverWebServer::handleOtaPage() {
  if (!authorized()) return;
  server.send_P(200, "text/html", OTA_PAGE);
}

void RoverWebServer::handleOtaUpload() {
  if (!authorized()) return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA: receiving %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA: %u bytes written\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void RoverWebServer::handleOtaUploadFinish() {
  if (Update.hasError()) {
    server.send(500, "text/plain", "Update failed");
  } else {
    server.send(200, "text/plain", "OK, rebooting");
    delay(500);
    ESP.restart();
  }
}

void RoverWebServer::handleSetMotorPWM() {
    if (!authorized()) return;
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body not found");
        return;
    }
    try {
        auto j = json::parse(server.arg("plain").c_str());
        if (!j.contains("pwm") || !j.contains("motor")) {
            server.send(400, "text/plain", "Missing 'pwm' or 'motor'");
            return;
        }
        MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
        int pwmValue = std::stoi(j["pwm"].get<std::string>());
        carController.setMotorSpeed(selection, pwmValue);
        server.send(204);
    } catch (const std::exception& e) {
        server.send(400, "text/plain", String("Invalid JSON: ") + e.what());
    }
}

void RoverWebServer::handleReboot() {
    if (!authorized()) return;
    server.send(200, "text/plain", "Rebooting");
    carController.reboot();
}

void RoverWebServer::handleGetWiFi() {
    if (!authorized()) return;
    auto wifiConfigMap = carController.getWiFiConfig();
    sendData(wifiConfigMap);
}

void RoverWebServer::handleWiFiForget() {
    if (!authorized()) return;
    carController.forgotWiFi();
    server.send(200, "text/plain", "Wi-Fi config forgotten. Restarting...");
    carController.reboot();
}

void RoverWebServer::handleMotorState() {
  if (!authorized()) return;
  sendData(carController.getMotorState());
}

void RoverWebServer::handleRoot() {
  if (!authorized()) return;
  server.send(200, "text/html", (const char*)control_index_html);
}

void RoverWebServer::handleSetMotor() {
    if (!authorized()) return;
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body not found");
        return;
    }
    try {
        auto j = json::parse(server.arg("plain").c_str());
        if (!j.contains("action") || !j.contains("motor")) {
            server.send(400, "text/plain", "Missing 'action' or 'motor'");
            return;
        }
        MotorAction action = static_cast<MotorAction>(j["action"].get<int>());
        MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
        carController.setMotorAction(action, selection);
        server.send(204);
    } catch (const std::exception& e) {
        server.send(400, "text/plain", String("Invalid JSON: ") + e.what());
    }
}

void RoverWebServer::handleGetUltrasonic() {
  if (!authorized()) return;
  sendData(carController.getUltrasonicState());
}

void RoverWebServer::handleCamera() {
    if (!authorized()) return;
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body not found");
        return;
    }
    try {
        auto j = json::parse(server.arg("plain").c_str());
        if (j.contains("frame_size")) {
            int size = j["frame_size"].get<int>();
            sensor_t *s = esp_camera_sensor_get();
            if (s && s->pixformat == PIXFORMAT_JPEG) {
                s->set_framesize(s, (framesize_t)size);
            }
        }
        server.send(204);
    } catch (const std::exception& e) {
        server.send(400, "text/plain", String("Invalid JSON: ") + e.what());
    }
}

void RoverWebServer::handleSystem() {
  if (!authorized()) return;
  sendData(systemMonitor.getState());
}

void RoverWebServer::handleStatus() {
    if (!authorized()) return;
    std::map<std::string, std::any> result;
    result["motor"] = carController.getMotorState();
    result["ultrasonic"] = carController.getUltrasonicState();
    result["system"] = systemMonitor.getState();
    sendData(result);
}

void RoverWebServer::handleConfig() {
    if (!authorized()) return;
    std::map<std::string, std::any> result;
    result["leftMotorPin1"] = config.leftMotorPin1;
    result["leftMotorPin2"] = config.leftMotorPin2;
    result["leftMotorPwm"] = config.leftMotorPwm;
    result["rightMotorPin1"] = config.rightMotorPin1;
    result["rightMotorPin2"] = config.rightMotorPin2;
    result["rightMotorPwm"] = config.rightMotorPwm;
    result["ultrasonicPin1"] = config.ultrasonicPin1;
    result["ultrasonicPin2"] = config.ultrasonicPin2;
    sendData(result);
}

void RoverWebServer::handleClient() {
  server.handleClient();
}

json RoverWebServer::asJSON(const std::map<std::string, std::any>& map) const {
    json j;
    for (const auto& [key, value] : map) {
        if (auto p = std::any_cast<int>(&value))           j[key] = *p;
        else if (auto p = std::any_cast<float>(&value))    j[key] = *p;
        else if (auto p = std::any_cast<bool>(&value))     j[key] = *p;
        else if (auto p = std::any_cast<std::string>(&value)) j[key] = *p;
        else if (auto p = std::any_cast<String>(&value))   j[key] = p->c_str();
        else if (auto p = std::any_cast<std::map<std::string, std::any>>(&value)) j[key] = asJSON(*p);
        // unknown type → silently skipped, same as before
    }
    return j;
}



void RoverWebServer::sendData(const std::map<std::string, std::any>& dataMap, const String& responseType) {
    json j = asJSON(dataMap);
    String response = j.dump().c_str();
    server.send(200, responseType.c_str(), response);
}