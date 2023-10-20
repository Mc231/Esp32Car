#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include "ServoControl.h"
#include "ServoManager.h"
#include "AbstractControl.h"  // Include the AbstractControl header
#include <map>
#include <any>

enum ServoSelection {
  HORIZONTAL,
  VERTICAL
};

class ServoControl : public AbstractControl {
public:
  ServoControl(ServoManager& horizontalServo, ServoManager& verticalServo);
  void changePosition(int position, ServoSelection selection);
  std::map<std::string, std::any> getState() const override;  // Implement the getState method from AbstractControl
private:
  ServoManager& horizontalServo;
  ServoManager& verticalServo;
};

#endif // MOTOR_CONTROL_H
