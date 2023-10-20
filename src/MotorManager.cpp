#include "MotorManager.h"

MotorManager::MotorManager(int pinA, int pinB, int pwm) {
  this->pinA = pinA;
  this->pinB = pinB;
  this->pwm = pwm;
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pwm, OUTPUT);
  currentAction = STOP;
  currentSpeed = 0;
  action(STOP);
}

void MotorManager::action(MotorAction act) {
  currentAction = act;
  switch (act) {
    case FORWARD:
      digitalWrite(pinA, HIGH);
      digitalWrite(pinB, LOW);
      analogWrite(pwm, currentSpeed); 
      break;
    case BACKWARD:
      digitalWrite(pinA, LOW);
      digitalWrite(pinB, HIGH);
      analogWrite(pwm, currentSpeed); 
      break;
    case STOP:
      digitalWrite(pinA, LOW);
      digitalWrite(pinB, LOW);
      analogWrite(pwm, currentSpeed); 
      break;
  }
}

void MotorManager::changeSpeed(int speed) {
  currentSpeed = constrain(speed, 0, 255); // Ensure speed is between 0 and 255
  if(currentAction == FORWARD || currentAction == BACKWARD) {
    analogWrite(pwm, currentSpeed); // Apply the PWM only if the motor is moving
  }
}

int MotorManager::getCurrentSpeed() {
  return currentSpeed;
}

MotorAction MotorManager::getCurrentAction() {
  return currentAction;
}
