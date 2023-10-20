#include "ServoControl.h"

ServoControl::ServoControl(ServoManager& horizontalServo, ServoManager& verticalServo)
  : horizontalServo(horizontalServo), verticalServo(verticalServo) {}

void ServoControl::changePosition(int position, ServoSelection selection) {
  switch (selection) {
    case HORIZONTAL:
      horizontalServo.setPosition(position);
      break;
    case VERTICAL:
      verticalServo.setPosition(position);
      break;
  }
}

std::map<std::string, std::any> ServoControl::getState() const {
  std::map<std::string, std::any> state;
  int horizontalPosition = horizontalServo.getPosition();
  int verticalPosition = verticalServo.getPosition();
  state["hp"] = horizontalPosition;
  state["vp"] = verticalPosition;
  return state;
}
