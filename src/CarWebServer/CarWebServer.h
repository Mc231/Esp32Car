#ifndef CARWEBSERVER_H
#define CARWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Controller/CarController.h"
#include <ElegantOTA.h>
#include "esp_camera.h"
#include "Config/ConfigManager.h"

class CarWebServer {
public:
  CarWebServer(CarController& carController, ConfigManager& configManager);
  void begin();
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

#endif // CARWEBSERVER_H
