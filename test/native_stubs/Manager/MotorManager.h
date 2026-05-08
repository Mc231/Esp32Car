// Native stub for MotorManager. Records the calls made by MotorControl
// so tests can verify routing (left vs. right vs. both) and PWM
// propagation without needing Arduino LEDC. Provides the same MotorAction
// enum that production code expects.
#ifndef NATIVE_MOTORMANAGER_STUB_H
#define NATIVE_MOTORMANAGER_STUB_H

enum MotorAction {
  FORWARD,
  BACKWARD,
  STOP
};

class MotorManager {
public:
  // Match the real ctor signature so MotorControl ctor compiles either
  // way; the args are ignored by the stub.
  MotorManager(int = 0, int = 0, int = 0, int = 0)
    : currentAction(STOP), currentSpeed(0),
      actionCalls(0), speedCalls(0), beginCalls(0) {}

  void begin() { beginCalls++; }
  void action(MotorAction act) { actionCalls++; currentAction = act; }
  void changeSpeed(int speed) { speedCalls++; currentSpeed = speed; }
  MotorAction getCurrentAction() { return currentAction; }
  int getCurrentSpeed() { return currentSpeed; }

  MotorAction currentAction;
  int currentSpeed;
  int actionCalls;
  int speedCalls;
  int beginCalls;
};

#endif
