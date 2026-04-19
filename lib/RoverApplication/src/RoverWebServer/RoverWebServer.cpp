#include "RoverWebServer.h"
#include "Html/control_html.h"

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
  server.begin(config.webServerPort);
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