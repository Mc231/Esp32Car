#include "RoverController.h"
#include <WiFi.h>

RoverController::RoverController(WiFiConfigManager& wiFiConfigManager, MotorControl& motorControl, UltrasonicManager& ultrasonicManager)
  : wiFiConfigManager(wiFiConfigManager),
    motorControl(motorControl),
    ultrasonicManager(ultrasonicManager),
    stateMutex(xSemaphoreCreateMutex())
{}

void RoverController::forgotWiFi() {
  Lock lk(stateMutex);
  wiFiConfigManager.clearConfig();
}

void RoverController::reboot() {
  delay(1000);
  ESP.restart();
}

std::map<std::string, std::any> RoverController::getWiFiConfig() {
  Lock lk(stateMutex);
  auto config = wiFiConfigManager.readConfig();
  IPAddress ip = WiFi.localIP();
  String ipAddress = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  return {{"ssid", config.ssid}, {"ip", ipAddress}};
}

void RoverController::setMotorSpeed(MotorSelection motorSelection, int speed) {
  Lock lk(stateMutex);
  motorControl.setPWM(motorSelection, speed);
}

void RoverController::setMotorAction(MotorAction action, MotorSelection motorSelection) {
  Lock lk(stateMutex);
  motorControl.action(action, motorSelection);
}

std::map<std::string, std::any> RoverController::getMotorState() {
  Lock lk(stateMutex);
  return motorControl.getState();
}

std::map<std::string, std::any> RoverController::getUltrasonicState() {
  Lock lk(stateMutex);
  return ultrasonicManager.getState();
}
