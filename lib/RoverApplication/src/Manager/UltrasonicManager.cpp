#include "UltrasonicManager.h"
#include <Arduino.h>

UltrasonicManager* UltrasonicManager::instance = nullptr;

UltrasonicManager::UltrasonicManager(int trigPin, int echoPin)
  : trigPin(trigPin), echoPin(echoPin), lastDistance(0.0f) {
}

void UltrasonicManager::initialize() {
  instance = this;
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  attachInterrupt(digitalPinToInterrupt(echoPin), &UltrasonicManager::echoISR, CHANGE);
  lastTriggerUs = micros();
  triggerPulse();
}

void IRAM_ATTR UltrasonicManager::echoISR() {
  if (!instance) return;
  if (digitalRead(instance->echoPin) == HIGH) {
    instance->echoStartUs = micros();
  } else {
    instance->echoEndUs = micros();
    instance->measurementReady.store(true, std::memory_order_release);
  }
}

void UltrasonicManager::triggerPulse() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  lastTriggerUs = micros();
}

void UltrasonicManager::update() {
  if (measurementReady.load(std::memory_order_acquire)) {
    unsigned long duration = echoEndUs - echoStartUs;
    lastDistance = (float)duration * 0.0344f / 2.0f;
    measurementReady.store(false, std::memory_order_release);
    triggerPulse();
    return;
  }
  // Timeout: echo never arrived (out of range or sensor disconnected).
  if ((micros() - lastTriggerUs) > (MEASUREMENT_INTERVAL_US + ECHO_TIMEOUT_US)) {
    triggerPulse();
  }
}

float UltrasonicManager::getLastDistance() const {
  return lastDistance;
}

std::map<std::string, std::any> UltrasonicManager::getState() {
  std::map<std::string, std::any> state;
  state["last_distance"] = lastDistance;
  return state;
}
