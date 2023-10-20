#include "HttpManager.h"
#include <ArduinoJson.h>
#include "control_html.h"

HttpManager::HttpManager(CarController& carController, ConfigManager& configManager)
  : carController(carController), configManager(configManager), server(32231) // Initialize motorControl here
{}
 
void HttpManager::begin() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); }); 
  server.on("/setServo", HTTP_PUT, [this]() { handleSetServo(); });
  server.on("/getServo", HTTP_GET, [this]() { handleGetServo(); });
  server.on("/setMotor", HTTP_PUT, [this]() { handleSetMotor(); }); 
  server.on("/getMotor", HTTP_GET, [this]() { handleMotorState(); }); 
  server.on("/getUltrasonic", HTTP_GET, [this]() { handleGetUltrasonic(); });
  server.on("/setMotorPWM", HTTP_PUT, [this]() { handleSetMotorPWM(); });
  server.on("/camera", HTTP_PUT, [this]() { handleCamera(); });
  ElegantOTA.begin(&server); 
  server.begin(32231);
}

void HttpManager::handleSetMotorPWM() {
  if (server.hasArg("plain") == false) {
    server.send(404, "text/plain", "Body not found");
    return;
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, server.arg("plain"));
  
  if (doc.containsKey("pwm") && doc.containsKey("motor")) {
    MotorSelection selection = static_cast<MotorSelection>(int(doc["motor"]));
    carController.setMotorSpeed(selection, doc["pwm"]);
  
  }
  server.send(204);
}

void HttpManager::handleMotorState() {
  DynamicJsonDocument doc = asJSON(512, carController.getMotorState());
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}


void HttpManager::handleRoot() {

  server.send(200, "text/html", (const char*)control_index_html);
}

void HttpManager::handleSetServo() {
  if (server.hasArg("plain") == false) {
    server.send(404, "text/plain", "Body not found");
    return;
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, server.arg("plain"));
  if (doc.containsKey("pos") && doc.containsKey("servo")) {
    int pos = doc["pos"];
    ServoSelection selection = static_cast<ServoSelection>(int(doc["servo"]));
    carController.setServoPosition(pos, selection);
  }
  server.send(204);
}

void HttpManager::handleGetServo() {
  DynamicJsonDocument doc = asJSON(512, carController.getServoState());
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void HttpManager::handleSetMotor() {
  if (server.hasArg("plain") == false) {
    server.send(404, "text/plain", "Body not found");
    return;
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, server.arg("plain"));
  
  if (doc.containsKey("action") && doc.containsKey("motor")) {
    MotorAction action = static_cast<MotorAction>(int(doc["action"]));
    MotorSelection selection = static_cast<MotorSelection>(int(doc["motor"]));
    carController.setMotorAction(action, selection);
  }

  server.send(204);
}

void HttpManager::handleGetUltrasonic() {
  float lastDistance = carController.getLastUltrasonicDistance();
    DynamicJsonDocument doc(256);
  doc["last_distance"] = lastDistance;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void HttpManager::handleCamera() {
    if (server.hasArg("plain") == false) {
      server.send(404, "text/plain", "Body not found");
      return;
    }

    DynamicJsonDocument doc(1024);
    int res = 0;
    deserializeJson(doc, server.arg("plain"));
  
    if (doc.containsKey("frame_size")) {
      int size = static_cast<int>(int(doc["frame_size"]));
      sensor_t *s = esp_camera_sensor_get();
      if (s->pixformat == PIXFORMAT_JPEG) {
            res = s->set_framesize(s, (framesize_t)size);
      }
    }
    DynamicJsonDocument responseDoc(1024);
    responseDoc["frame_size"] = res;
    String response;
    serializeJson(responseDoc, response);
    server.send(204, "application/json", response);
}

void HttpManager::handleClient() {
  server.handleClient();
  ElegantOTA.loop();
}

  DynamicJsonDocument HttpManager::asJSON(size_t docSize, std::map<std::string, std::any> map) const {
        DynamicJsonDocument doc(docSize);

        for (const auto& item : map) {
            const auto& key = item.first;
            const auto& value = item.second;

            try {
                // Try casting to String
                doc[key.c_str()] = std::any_cast<String>(value);
            } catch (const std::bad_any_cast&) {
                try {
                    // If it fails, try casting to int
                    doc[key.c_str()] = std::any_cast<int>(value);
                } catch (const std::bad_any_cast&) {
                    // If both casts fail, you can handle the error here or ignore it.
                    // Optionally handle other types.
                }
            }
        }

        return doc;
    }