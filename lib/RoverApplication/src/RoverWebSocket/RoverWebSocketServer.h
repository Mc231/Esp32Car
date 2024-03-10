#ifndef ROVERWEBSOCKETSERVER_H
#define ROVERWEBSOCKETSERVER_H

#include <Arduino.h>
#include "Controller/RoverController.h"
#include "esp_camera.h"
#include "Config/RoverApplicationConfig.h"
#include "Monitor/SystemMonitor.h"
#include <WebSocketsServer.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RoverWebSocketServer {
public:
  RoverWebSocketServer(RoverController& carController, RoverApplicationConfig& config, SystemMonitor& systemMonitor);
  void begin();
  void loop();
private:
  WebSocketsServer wsServer;
  RoverController &carController;
  RoverApplicationConfig config;
  SystemMonitor &systemMonitor;
  void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
  json asJSON(const std::map<std::string, std::any>& map) const;
  void sendData(const std::map<std::string, std::any>& dataMap);
  void handleSystem();
  void handleStatus();
  void handleConfig();
  void handleReboot();
  void handleGetWiFi();
  void handleWiFiForget();
  void handleGetUltrasonic();
  void handleCamera(const nlohmann::json& j);
  void handleSetMotorPWM(const nlohmann::json& j);
  void handleMotorState();
  void handleSetMotor(const nlohmann::json& j); 

};

#endif // ROVERWEBSOCKETSERVER_H
