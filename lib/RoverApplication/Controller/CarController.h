#ifndef CARCONTROLLER_H
#define CARCONTROLLER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Control/MotorControl.h"
#include "Manager/UltrasonicManager.h"
#include "WiFi/WiFiConfigManager.h"

class CarController {
public:
  CarController(WiFiConfigManager& wiFiConfigManager, MotorControl& motorControl, UltrasonicManager& ultrasonicManager);
  void forgotWiFi();
  void reboot();
  void setMotorSpeed(MotorSelection motorSelection, int speed);
  void setMotorAction(MotorAction action, MotorSelection motorSelection);
  String getWiFiConfig();
  std::map<std::string, std::any> getMotorState();
  std::map<std::string, std::any> getUltrasonicState();
private:
  WiFiConfigManager& wiFiConfigManager;
  MotorControl& motorControl;
  UltrasonicManager& ultrasonicManager;
};

#endif // CARCONTROLLER
