#include "RoverController.h"
#include <WiFi.h>

RoverController::RoverController(WiFiConfigManager& wiFiConfigManager, MotorControl& motorControl, UltrasonicManager& ultrasonicManager)
  : wiFiConfigManager(wiFiConfigManager), motorControl(motorControl), ultrasonicManager(ultrasonicManager)
{}

  void RoverController::forgotWiFi() {
    wiFiConfigManager.clearConfig();
  }

  void RoverController::reboot() {
    delay(1000);
    ESP.restart();
  }

  std::map<std::string, std::any> RoverController::getWiFiConfig() {
    auto config = wiFiConfigManager.readConfig();
    IPAddress ip = WiFi.localIP();
    String ipAddress = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    return {{"ssid", config.ssid}, {"ip", ipAddress}};
  }

  void RoverController::setMotorSpeed(MotorSelection motorSelection, int speed) {
    motorControl.setPWM(motorSelection, speed);
  }

  void RoverController::setMotorAction(MotorAction action, MotorSelection motorSelection) {
    motorControl.action(action, motorSelection);
  }

  std::map<std::string, std::any> RoverController::getMotorState() {
    return motorControl.getState();
  }

  std::map<std::string, std::any> RoverController::getUltrasonicState() {
    return ultrasonicManager.getState();
  }
