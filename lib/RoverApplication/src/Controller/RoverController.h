#ifndef ROVERCONTROLLER_H
#define ROVERCONTROLLER_H

#include <Arduino.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Control/MotorControl.h"
#include "Manager/DistanceManager.h"
#include "WiFi/WiFiConfigManager.h"

class RoverController {
public:
  RoverController(WiFiConfigManager& wiFiConfigManager, MotorControl& motorControl, DistanceManager& distanceManager);
  void forgotWiFi();
  void reboot();
  void setMotorSpeed(MotorSelection motorSelection, int speed);
  void setMotorAction(MotorAction action, MotorSelection motorSelection);
  std::map<std::string, std::any> getWiFiConfig();
  std::map<std::string, std::any> getMotorState();
  std::map<std::string, std::any> getDistanceState();

  // Deadman: call from the main loop. If no driving command has arrived
  // within deadmanTimeoutMs the motors are stopped. 0 = disabled.
  void setDeadmanTimeout(unsigned long timeoutMs) { deadmanTimeoutMs = timeoutMs; }
  void tickDeadman();
private:
  WiFiConfigManager& wiFiConfigManager;
  MotorControl& motorControl;
  DistanceManager& distanceManager;
  SemaphoreHandle_t stateMutex;
  unsigned long lastCommandMs = 0;
  unsigned long deadmanTimeoutMs = 0;
  bool motorsActive = false;

  // RAII helper for the mutex.
  struct Lock {
    SemaphoreHandle_t& m;
    Lock(SemaphoreHandle_t& m) : m(m) { xSemaphoreTake(m, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(m); }
  };
};

#endif // ROVERCONTROLLER
