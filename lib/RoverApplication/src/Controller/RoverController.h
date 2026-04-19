#ifndef ROVERCONTROLLER_H
#define ROVERCONTROLLER_H

#include <Arduino.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Control/MotorControl.h"
#include "Manager/UltrasonicManager.h"
#include "WiFi/WiFiConfigManager.h"

class RoverController {
public:
  RoverController(WiFiConfigManager& wiFiConfigManager, MotorControl& motorControl, UltrasonicManager& ultrasonicManager);
  void forgotWiFi();
  void reboot();
  void setMotorSpeed(MotorSelection motorSelection, int speed);
  void setMotorAction(MotorAction action, MotorSelection motorSelection);
  std::map<std::string, std::any> getWiFiConfig();
  std::map<std::string, std::any> getMotorState();
  std::map<std::string, std::any> getUltrasonicState();
private:
  WiFiConfigManager& wiFiConfigManager;
  MotorControl& motorControl;
  UltrasonicManager& ultrasonicManager;
  SemaphoreHandle_t stateMutex;

  // RAII helper for the mutex.
  struct Lock {
    SemaphoreHandle_t& m;
    Lock(SemaphoreHandle_t& m) : m(m) { xSemaphoreTake(m, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(m); }
  };
};

#endif // ROVERCONTROLLER
