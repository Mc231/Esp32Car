#include "RoverWebServer.h"
#include "Html/control_html.h"

RoverWebServer::RoverWebServer(RoverController& carController, RoverApplicationConfig config, SystemMonitor&)
  : carController(carController), config(config), systemMonitor(systemMonitor), server(config.webServerPort) 
{}
 
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
    if (server.hasArg("plain") == false) {
        server.send(404, "text/plain", "Body not found");
        return;
    }
    auto j = json::parse(server.arg("plain").c_str());
    Serial.println(j.dump().c_str());
    if (j.contains("pwm") && j.contains("motor")) {
        int motor = j["motor"].get<int>();
        MotorSelection selection = static_cast<MotorSelection>(motor);
    
        int pwmValue = std::stoi(j["pwm"].get<std::string>());
        carController.setMotorSpeed(selection, pwmValue);
    }
    server.send(204);
}

void RoverWebServer::handleReboot() {
    server.send(200, "text/plain", "Rebooting");
    carController.reboot();
}

void RoverWebServer::handleGetWiFi() {
    String response = carController.getWiFiConfig();
    server.send(200, "application/json", response);
}

void RoverWebServer::handleWiFiForget() {
    carController.forgotWiFi();
    server.send(200, "text/plain", "Wi-Fi config forgotten. Restarting...");
    carController.reboot();
}

void RoverWebServer::handleMotorState() {
  sendData(carController.getMotorState());
}

void RoverWebServer::handleRoot() {
  server.send(200, "text/html", (const char*)control_index_html);
}

void RoverWebServer::handleSetMotor() {
    if (server.hasArg("plain") == false) {
        server.send(404, "text/plain", "Body not found");
        return;
    }

    auto j = json::parse(server.arg("plain").c_str());
    Serial.println(j.dump().c_str());
    if (j.contains("action") && j.contains("motor")) {
        MotorAction action = static_cast<MotorAction>(j["action"].get<int>());
        MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
        carController.setMotorAction(action, selection);
    }

    server.send(204);
}

void RoverWebServer::handleGetUltrasonic() {
  sendData(carController.getUltrasonicState());
}

void RoverWebServer::handleCamera() {
    if (!server.hasArg("plain")) {
        server.send(404, "text/plain", "Body not found");
        return;
    }

    auto j = json::parse(server.arg("plain").c_str());

    if (j.contains("frame_size")) {
        int size = j["frame_size"].get<int>();
        sensor_t *s = esp_camera_sensor_get();
        if (s->pixformat == PIXFORMAT_JPEG) {
            s->set_framesize(s, (framesize_t)size);
        }
    }
    server.send(204);
}

void RoverWebServer::handleSystem() {
  sendData(systemMonitor.getState());
}

void RoverWebServer::handleStatus() {
    std::map<std::string, std::any> result;
    result["motor"] = carController.getMotorState();
    result["ultrasonic"] = carController.getUltrasonicState();
    result["system"] = systemMonitor.getState();
    sendData(result);
}

void RoverWebServer::handleConfig() {
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
    for (const auto& item : map) {
        const auto& key = item.first;
        const auto& value = item.second;

        try {
            j[key] = std::any_cast<String>(value).c_str();
        } catch (const std::bad_any_cast&) {
            try {
                j[key] = std::any_cast<std::string>(value);
            } catch (const std::bad_any_cast&) {
                try {
                    j[key] = std::any_cast<int>(value);
                } catch (const std::bad_any_cast&) {
                    try {
                        j[key] = std::any_cast<float>(value);
                    } catch (const std::bad_any_cast&) {
                        try {
                            // This handles a nested std::map<std::string, std::any>
                            j[key] = asJSON(std::any_cast<std::map<std::string, std::any>>(value));
                        } catch (const std::bad_any_cast&) {
                            // Handle other types or ignore.
                        }
                    }
                }
            }
        }
    }
    return j;
}



void RoverWebServer::sendData(const std::map<std::string, std::any>& dataMap, const String& responseType) {
    json j = asJSON(dataMap);
    String response = j.dump().c_str();
    server.send(200, responseType.c_str(), response);
}