#include "CarController.h"
#include <ArduinoJson.h>

CarController::CarController(ServoControl& servoControl, MotorControl& motorControl, UltrasonicManager& ultrasonicManager)
  : servoControl(servoControl), motorControl(motorControl), ultrasonicManager(ultrasonicManager)
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

  void CarController::setServoPosition(int position, ServoSelection selection) {
    servoControl.changePosition(position, selection);
  }

  std::map<std::string, std::any> CarController::getServoState() {
    return servoControl.getState();
  }


  float CarController::getLastUltrasonicDistance() {
    return ultrasonicManager.getDistance();
  }
