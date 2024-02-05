#ifndef ROVERWEBSERVER_H
#define ROVERWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Controller/RoverController.h"
#include "esp_camera.h"
#include "Config/RoverApplicationConfig.h"
#include "Monitor/SystemMonitor.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RoverWebServer {
public:
  RoverWebServer(RoverController& carController, RoverApplicationConfig config, SystemMonitor&);
  void begin();
  void handleReboot();
  void handleGetWiFi();
  void handleWiFiForget();
  void handleSetMotor(); 
  void handleSetMotorPWM();
  void handleMotorState();
  void handleClient();
  void handleGetUltrasonic();
  void handleCamera();
  void handleRoot();
  void handleSystem();
  void handleStatus();
  void handleConfig();
private:
  RoverController &carController;
  RoverApplicationConfig config;
  SystemMonitor &systemMonitor;
  WebServer server;
  json asJSON(const std::map<std::string, std::any>& map) const;
  void sendData(const std::map<std::string, std::any>& dataMap, const String& responseType = "application/json");
};

#endif // ROVERWEBSERVER_H
