// Native stub of RoverController: same public API as the real class,
// but in-memory and free of FreeRTOS / hardware deps. Tests inspect
// `lastSetAction` / `lastSetSpeed` / `rebootCount` etc. to verify the
// dispatcher routed commands correctly. The state-getter return values
// are programmable so each test can shape the response shape.
#ifndef NATIVE_ROVERCONTROLLER_STUB_H
#define NATIVE_ROVERCONTROLLER_STUB_H

#include <map>
#include <any>
#include <string>
#include "Control/MotorControl.h"

class RoverController {
public:
  // Programmable canned responses (set by tests):
  std::map<std::string, std::any> motorState;
  std::map<std::string, std::any> distanceState;
  std::map<std::string, std::any> wiFiConfig;

  // Call recorders:
  int   forgotWiFiCount = 0;
  int   rebootCount = 0;
  bool  setMotorSpeedCalled = false;
  int   lastSetSpeed = 0;
  MotorSelection lastSetSpeedMotor = BOTH_MOTORS;
  bool  setMotorActionCalled = false;
  MotorAction    lastSetAction = STOP;
  MotorSelection lastSetActionMotor = BOTH_MOTORS;

  void forgotWiFi() { forgotWiFiCount++; }
  void reboot() { rebootCount++; }

  void setMotorSpeed(MotorSelection m, int speed) {
    setMotorSpeedCalled = true;
    lastSetSpeedMotor = m;
    lastSetSpeed = speed;
  }
  void setMotorAction(MotorAction a, MotorSelection m) {
    setMotorActionCalled = true;
    lastSetAction = a;
    lastSetActionMotor = m;
  }

  std::map<std::string, std::any> getMotorState() { return motorState; }
  std::map<std::string, std::any> getDistanceState() { return distanceState; }
  std::map<std::string, std::any> getWiFiConfig() { return wiFiConfig; }
};

#endif
