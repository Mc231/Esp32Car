// ServoManager.h
#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <Arduino.h>
#include <Servo.h>

class ServoManager {
private:
  Servo myservo;
  int pos;
  int minPos;
  int maxPos;
  
public:
  ServoManager(int pin, int minPosition = 0, int maxPosition = 180);
  void setPosition(int newPosition);
  int getPosition();
};

#endif // SERVO_MANAGER_H