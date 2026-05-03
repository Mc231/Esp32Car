#include "DistanceManager.h"
#include "../Log/RemoteLogger.h"
#include <Arduino.h>
#include <math.h>

DistanceManager::DistanceManager(int signalPin)
  : signalPin(signalPin), lastDistance(0.0f) {
}

void DistanceManager::initialize() {
  // analogRead handles ADC pin setup automatically on ESP32-Arduino. The
  // GP2Y0A21 outputs at most ~3.1V at 10cm, so the default ADC_11db
  // attenuation (0-~3.6V range) is the right choice — set on first read.
  Log.printf("[ir-distance] init on GPIO %d (Sharp GP2Y0A21, 10-80cm)\n", signalPin);
}

void DistanceManager::update() {
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  // Oversample to suppress the ~38ms current-spike-induced noise on the rail.
  uint32_t mvSum = 0;
  for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
    mvSum += analogReadMilliVolts(signalPin);
  }
  float voltage = (float)mvSum / (OVERSAMPLE_COUNT * 1000.0f);  // V

  // GP2Y0A21 valid range: ~0.4V (80cm) to ~2.7V (10cm). Outside that:
  //   <0.4V → nothing within 80cm
  //   >2.7V → object inside 10cm blind zone (sensor curve folds back here);
  //           treat as "right at the bumper", report 10cm.
  if (voltage < 0.4f) {
    lastDistance = -1.0f;  // sentinel: out of range / no obstacle within 80cm
  } else {
    float d = 27.86f * powf(voltage, -1.15f);
    if (d < 10.0f) d = 10.0f;
    if (d > 80.0f) d = 80.0f;
    lastDistance = d;
  }
}

float DistanceManager::getLastDistance() const {
  return lastDistance;
}

std::map<std::string, std::any> DistanceManager::getState() {
  std::map<std::string, std::any> state;
  state["last_distance"] = lastDistance;
  return state;
}
