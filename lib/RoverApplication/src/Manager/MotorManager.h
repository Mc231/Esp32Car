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
    int ledcChannel;          // explicit LEDC channel — must NOT collide with camera (channel 0)
    MotorAction currentAction;
    int currentSpeed;

  public:
    // ledcChannel: pick channels 4..7 to avoid clashing with esp_camera's
    // LEDC_CHANNEL_0 (used for the OV2640 XCLK at 20MHz).
    MotorManager(int pinA, int pinB, int pwm, int ledcChannel);
    void begin();
    void action(MotorAction act);
    void changeSpeed(int speed);
    MotorAction getCurrentAction();
    int getCurrentSpeed();
};

#endif // MOTOR_MANAGER_H
