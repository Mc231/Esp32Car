#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include "CarController.h"
#include <ElegantOTA.h>
#include "esp_camera.h"
#include "ConfigManager.h"

class HttpManager {
public:
  HttpManager(CarController& carController, ConfigManager& configManager);
  void begin();
  void handleSetServo();
  void handleGetServo();
  void handleSetMotor(); 
  void handleSetMotorPWM();
  void handleMotorState();
  void handleClient();
  void handleGetUltrasonic();
  void handleCamera();
  void handleRoot();
private:
  CarController &carController;
  ConfigManager &configManager;
  WebServer server;
  DynamicJsonDocument asJSON(size_t docSize, std::map<std::string, std::any> map) const;
};

#endif // HTTPMANAGER_H
