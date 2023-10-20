#ifndef CARCONTROLLER_H
#define CARCONTROLLER_H

#include <Arduino.h>
#include <WebServer.h>
#include "ServoControl.h"
#include "ServoManager.h"
#include "MotorControl.h"
#include "UltrasonicManager.h"
#include <ElegantOTA.h>

class CarController {
public:
  CarController(ServoControl& servoControl, MotorControl& motorControl, UltrasonicManager& ultrasonicManager);
  void setMotorSpeed(MotorSelection motorSelection, int speed);
  void setMotorAction(MotorAction action, MotorSelection motorSelection);
  std::map<std::string, std::any> getMotorState();
  void setServoPosition(int position, ServoSelection selection);
  std::map<std::string, std::any> getServoState();
  float getLastUltrasonicDistance();
  

private:
  ServoControl& servoControl;
  MotorControl& motorControl;
  UltrasonicManager& ultrasonicManager;
};

#endif // CARCONTROLLER
