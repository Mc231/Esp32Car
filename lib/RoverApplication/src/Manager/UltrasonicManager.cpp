#include "UltrasonicManager.h"
#include "../Log/RemoteLogger.h"
#include <Arduino.h>

UltrasonicManager* UltrasonicManager::instance = nullptr;

UltrasonicManager::UltrasonicManager(int signalPin)
  : signalPin(signalPin), lastDistance(0.0f) {
}

void UltrasonicManager::initialize() {
  instance = this;
  Log.printf("[ultrasonic] init on GPIO %d (single-pin mode)\n", signalPin);
  triggerPulse();
}

void IRAM_ATTR UltrasonicManager::echoISR() {
  if (!instance) return;
  if (digitalRead(instance->signalPin) == HIGH) {
    instance->echoStartUs = micros();
  } else {
    instance->echoEndUs = micros();
    instance->measurementReady.store(true, std::memory_order_release);
  }
}

void UltrasonicManager::triggerPulse() {
  // Detach while we drive the pin as output so the ISR doesn't see our own edges.
  detachInterrupt(digitalPinToInterrupt(signalPin));
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, LOW);
  delayMicroseconds(2);
  digitalWrite(signalPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(signalPin, LOW);
  // Hand the line over to the sensor and listen for the echo pulse.
  pinMode(signalPin, INPUT_PULLDOWN);
  echoStartUs = 0;
  echoEndUs   = 0;
  measurementReady.store(false, std::memory_order_release);
  attachInterrupt(digitalPinToInterrupt(signalPin), &UltrasonicManager::echoISR, CHANGE);
  lastTriggerUs = micros();
}

void UltrasonicManager::update() {
  if (measurementReady.load(std::memory_order_acquire)) {
    unsigned long pulseUs = echoEndUs - echoStartUs;
    lastDistance = (float)pulseUs * 0.0344f / 2.0f;
    triggerPulse();
    return;
  }
  // Timeout: echo never arrived (out of range or wiring issue).
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
