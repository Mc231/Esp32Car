#include "MotorControl.h"

MotorControl::MotorControl(MotorManager& leftMotor, MotorManager& rightMotor)
  : leftMotor(leftMotor), rightMotor(rightMotor) {}

void MotorControl::action(MotorAction act, MotorSelection selection) {
  lastActionSelection = selection;  // Store the selection
  
  switch (selection) {
    case LEFT:
      leftMotor.action(act);
      break;
    case RIGHT:
      rightMotor.action(act);
      break;
    case ALL:
      leftMotor.action(act);
      rightMotor.action(act);
      break;
  }
}

void MotorControl::setPWM(MotorSelection selection, int pwmValue) {
  lastPWMSelection = selection;  // Store the selection
  
  switch (selection) {
    case LEFT:
      leftMotor.changeSpeed(pwmValue);
      break;
    case RIGHT:
      rightMotor.changeSpeed(pwmValue);
      break;
    case ALL:
      leftMotor.changeSpeed(pwmValue);
      rightMotor.changeSpeed(pwmValue);
      break;
  }
}

std::map<std::string, std::any> MotorControl::getState() const {
  std::map<std::string, std::any> state;
  
  MotorAction leftAction = leftMotor.getCurrentAction();
  MotorAction rightAction = rightMotor.getCurrentAction();
  
  state["lma"] = static_cast<int>(leftAction);
  state["rma"] = static_cast<int>(rightAction);
  state["las"] = static_cast<int>(lastActionSelection);  // Store the last MotorSelection for action
  
  state["lmpwm"] = leftMotor.getCurrentSpeed();
  state["rmpwm"] = rightMotor.getCurrentSpeed();
  state["lpwms"] = static_cast<int>(lastPWMSelection);  // Store the last MotorSelection for PWM

  return state;
}
