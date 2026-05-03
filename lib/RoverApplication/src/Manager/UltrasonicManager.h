#ifndef ULTRASONIC_MANAGER_H
#define ULTRASONIC_MANAGER_H

#include <Arduino.h>
#include <map>
#include <any>
#include <atomic>

class UltrasonicManager {
public:
  // Single-pin HC-SR04: TRIG and ECHO joined externally through a 1k resistor,
  // ESP32 pin tapped via 1k/2k divider so the 5V echo lands at ~3.3V.
  UltrasonicManager(int signalPin);
  void initialize();
  // Call from the main loop. Triggers a new measurement if the previous one
  // finished or timed out. Non-blocking.
  void update();
  float getLastDistance() const;
  std::map<std::string, std::any> getState();

private:
  int signalPin;
  float lastDistance;

  static constexpr unsigned long MEASUREMENT_INTERVAL_US = 60000;  // 60 ms between pings
  static constexpr unsigned long ECHO_TIMEOUT_US        = 30000;  // ~5 m max range

  // ISR-shared state — single instance assumption (one ultrasonic per board).
  static UltrasonicManager* instance;
  volatile unsigned long echoStartUs = 0;
  volatile unsigned long echoEndUs   = 0;
  std::atomic<bool> measurementReady{false};
  unsigned long lastTriggerUs = 0;

  static void IRAM_ATTR echoISR();
  void triggerPulse();
};

#endif // ULTRASONIC_MANAGER_H
