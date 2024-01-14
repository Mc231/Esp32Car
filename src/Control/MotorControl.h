#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "Manager/MotorManager.h"
#include "AbstractControl.h"  // Include the AbstractControl header
#include <map>
#include <any>

enum MotorSelection {
  LEFT,
  RIGHT,
  ALL
};

class MotorControl : public AbstractControl {  // Inherit from the AbstractControl
public:
  MotorControl(MotorManager& leftMotor, MotorManager& rightMotor);
  void action(MotorAction act, MotorSelection selection);
  void setPWM(MotorSelection selection, int pwmValue);
  std::map<std::string, std::any> getState() const override;  // Implement the getState method from AbstractControl
private:
  MotorManager& leftMotor;
  MotorManager& rightMotor;
  MotorSelection lastActionSelection = ALL;
  MotorSelection lastPWMSelection = ALL;
};

#endif // MOTOR_CONTROL_H
