#include "CarController.h"
#include <ArduinoJson.h>

CarController::CarController(MotorControl& motorControl, UltrasonicManager& ultrasonicManager)
  : motorControl(motorControl), ultrasonicManager(ultrasonicManager)
{}

  void CarController::setMotorSpeed(MotorSelection motorSelection, int speed) {
    motorControl.setPWM(motorSelection, speed);
  }

  void CarController::setMotorAction(MotorAction action, MotorSelection motorSelection) {
    motorControl.action(action, motorSelection);
  }

  std::map<std::string, std::any> CarController::getMotorState() {
    return motorControl.getState();
  }

  float CarController::getLastUltrasonicDistance() {
    return ultrasonicManager.getDistance();
  }
