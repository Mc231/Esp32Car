#include "CarWebServer.h"
#include "Html/control_html.h"

CarWebServer::CarWebServer(CarController& carController, ConfigManager& configManager, SystemMonitor&)
  : carController(carController), configManager(configManager), systemMonitor(systemMonitor), server(32231) // Initialize motorControl here
{}
 
void CarWebServer::begin() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); }); 
  server.on("/motor", HTTP_PUT, [this]() { handleSetMotor(); }); 
  server.on("/motor", HTTP_GET, [this]() { handleMotorState(); }); 
  server.on("/ultrasonic", HTTP_GET, [this]() { handleGetUltrasonic(); });
  server.on("/motorPWM", HTTP_PUT, [this]() { handleSetMotorPWM(); });
  server.on("/camera", HTTP_PUT, [this]() { handleCamera(); });
  server.on("/system", HTTP_GET, [this]() { handleSystem(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); } );
  server.on("/config", HTTP_GET, [this]() { handleConfig();} );
  ElegantOTA.begin(&server); 
  server.begin(32231);
}

void CarWebServer::handleSetMotorPWM() {
    if (server.hasArg("plain") == false) {
        server.send(404, "text/plain", "Body not found");
        return;
    }

    auto j = json::parse(server.arg("plain").c_str());

    if (j.contains("pwm") && j.contains("motor")) {
        MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
        carController.setMotorSpeed(selection, j["pwm"]);
    }
    server.send(204);
}

void CarWebServer::handleMotorState() {
  sendData(carController.getMotorState());
}

void CarWebServer::handleRoot() {
  server.send(200, "text/html", (const char*)control_index_html);
}

void CarWebServer::handleSetMotor() {
    if (server.hasArg("plain") == false) {
        server.send(404, "text/plain", "Body not found");
        return;
    }

    auto j = json::parse(server.arg("plain").c_str());

    if (j.contains("action") && j.contains("motor")) {
        MotorAction action = static_cast<MotorAction>(j["action"].get<int>());
        MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
        carController.setMotorAction(action, selection);
    }

    server.send(204);
}

void CarWebServer::handleGetUltrasonic() {
  sendData(carController.getUltrasonicState());
}

void CarWebServer::handleCamera() {
    if (!server.hasArg("plain")) {
        server.send(404, "text/plain", "Body not found");
        return;
    }

    auto j = json::parse(server.arg("plain").c_str());

    int res = 0;

    if (j.contains("frame_size")) {
        int size = j["frame_size"].get<int>();
        sensor_t *s = esp_camera_sensor_get();
        if (s->pixformat == PIXFORMAT_JPEG) {
            res = s->set_framesize(s, (framesize_t)size);
        }
    }

    json responseJ;
    responseJ["frame_size"] = res;
    String response = responseJ.dump().c_str();
    server.send(204, "application/json", response);
}

void CarWebServer::handleSystem() {
  sendData(systemMonitor.getState());
}

void CarWebServer::handleStatus() {
    std::map<std::string, std::any> result;
    result["motor"] = carController.getMotorState();
    result["ultrasonic"] = carController.getUltrasonicState();
    result["system"] = systemMonitor.getState();
    sendData(result);
}

void CarWebServer::handleConfig() {
    sendData(configManager.getConfig());
}

void CarWebServer::handleClient() {
  server.handleClient();
  ElegantOTA.loop();
}

json CarWebServer::asJSON(const std::map<std::string, std::any>& map) const {
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



void CarWebServer::sendData(const std::map<std::string, std::any>& dataMap, const String& responseType) {
    json j = asJSON(dataMap);
    String response = j.dump().c_str();
    server.send(200, responseType.c_str(), response);
}