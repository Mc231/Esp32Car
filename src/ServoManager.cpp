// ServoManager.cpp
#include "ServoManager.h"

ServoManager::ServoManager(int pin, int minPosition, int maxPosition) {
  myservo.attach(pin);
  pos = 0;
  minPos = minPosition;
  maxPos = maxPosition;
}

void ServoManager::setPosition(int newPosition) {
  if (newPosition >= minPos && newPosition <= maxPos) {
    pos = newPosition;
    myservo.write(pos);
  }
}

int ServoManager::getPosition() {
  return pos;
}