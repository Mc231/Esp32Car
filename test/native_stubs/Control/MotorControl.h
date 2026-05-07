// Native stub for MotorControl — only the enums CommandDispatcher needs.
#ifndef NATIVE_MOTORCONTROL_STUB_H
#define NATIVE_MOTORCONTROL_STUB_H

enum MotorAction {
  FORWARD = 0,
  BACKWARD = 1,
  STOP = 2,
};

enum MotorSelection {
  LEFT_MOTOR  = 0,
  RIGHT_MOTOR = 1,
  BOTH_MOTORS = 2,
};

#endif
