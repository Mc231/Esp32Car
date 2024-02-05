#include "CarController.h"
#include <WiFi.h>

CarController::CarController(WiFiConfigManager& wiFiConfigManager, MotorControl& motorControl, UltrasonicManager& ultrasonicManager)
  : wiFiConfigManager(wiFiConfigManager), motorControl(motorControl), ultrasonicManager(ultrasonicManager)
{}

  void CarController::forgotWiFi() {
    wiFiConfigManager.clearConfig();
  }

  void CarController::reboot() {
    delay(1000);
    ESP.restart();
  }

  String CarController::getWiFiConfig() {
    auto config = wiFiConfigManager.readConfig();
    IPAddress ip = WiFi.localIP(); // Get local IP address
    String ipAddress = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

    // Construct and return the JSON string
    return "{\"ssid\":\"" + config.ssid + "\", \"ip\":\"" + ipAddress + "\"}";
  }

  void CarController::setMotorSpeed(MotorSelection motorSelection, int speed) {
    motorControl.setPWM(motorSelection, speed);
  }

  void CarController::setMotorAction(MotorAction action, MotorSelection motorSelection) {
    motorControl.action(action, motorSelection);
  }

  std::map<std::string, std::any> CarController::getMotorState() {
    return motorControl.getState();
  }

  std::map<std::string, std::any> CarController::getUltrasonicState() {
    return ultrasonicManager.getState();
  }
