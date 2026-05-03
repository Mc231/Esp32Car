#ifndef DISTANCE_MANAGER_H
#define DISTANCE_MANAGER_H

#include <Arduino.h>
#include <map>
#include <any>
#include <string>

// Drives a Sharp GP2Y0A21YK0F IR distance sensor (analog output, 10-80cm).
class DistanceManager {
public:
  DistanceManager(int signalPin);
  void initialize();
  void update();
  float getLastDistance() const;
  std::map<std::string, std::any> getState();

private:
  int signalPin;
  float lastDistance;
  unsigned long lastSampleMs = 0;

  static constexpr unsigned long SAMPLE_INTERVAL_MS = 50;   // ~20 Hz
  static constexpr int           OVERSAMPLE_COUNT   = 8;    // average N reads to smooth
};

#endif // DISTANCE_MANAGER_H
