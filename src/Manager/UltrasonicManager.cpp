#include "UltrasonicManager.h"
#include <Arduino.h>

UltrasonicManager::UltrasonicManager(int trigPin, int echoPin)
  : trigPin(trigPin), echoPin(echoPin), lastDistance(0.0) {
}

void UltrasonicManager::initialize() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

float UltrasonicManager::getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  lastDistance = (float)duration * 0.0344 / 2.0;
  return lastDistance;
}

float UltrasonicManager::getLastDistance() const {
  return lastDistance;
}

std::map<std::string, std::any> UltrasonicManager::getState() {
    // TODO: - Do some timer to get value
    this->lastDistance = getDistance();
    std::map<std::string, std::any> state;
    state["last_distance"] = lastDistance;
    return state;
}