#ifndef CARCONTROLLER_H
#define CARCONTROLLER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Control/MotorControl.h"
#include "Manager/UltrasonicManager.h"
#include <ElegantOTA.h>

class CarController {
public:
  CarController(MotorControl& motorControl, UltrasonicManager& ultrasonicManager);
  void setMotorSpeed(MotorSelection motorSelection, int speed);
  void setMotorAction(MotorAction action, MotorSelection motorSelection);
  std::map<std::string, std::any> getMotorState();
  float getLastUltrasonicDistance();
  

private:
  MotorControl& motorControl;
  UltrasonicManager& ultrasonicManager;
};

#endif // CARCONTROLLER
