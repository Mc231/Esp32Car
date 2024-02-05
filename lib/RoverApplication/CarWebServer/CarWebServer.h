#ifndef CARWEBSERVER_H
#define CARWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Controller/CarController.h"
#include "esp_camera.h"
#include "Config/CarApplicationConfig.h"
#include "Monitor/SystemMonitor.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CarWebServer {
public:
  CarWebServer(CarController& carController, CarApplicationConfig config, SystemMonitor&);
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
  CarController &carController;
  CarApplicationConfig config;
  SystemMonitor &systemMonitor;
  WebServer server;
  json asJSON(const std::map<std::string, std::any>& map) const;
  void sendData(const std::map<std::string, std::any>& dataMap, const String& responseType = "application/json");
};

#endif // CARWEBSERVER_H
