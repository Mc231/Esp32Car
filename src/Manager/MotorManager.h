#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include "Arduino.h"

enum MotorAction {
  FORWARD,
  BACKWARD,
  STOP
};

class MotorManager {
  private:
    int pinA;
    int pinB;
    int pwm;
    MotorAction currentAction;
    int currentSpeed;

  public:
    MotorManager(int pinA, int pinB, int pwm);
    void action(MotorAction act);
    void changeSpeed(int speed);
    MotorAction getCurrentAction();
    int getCurrentSpeed();
};

#endif // MOTOR_MANAGER_H
