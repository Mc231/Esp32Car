#ifndef BLEMANAGER_H
#define BLEMANAGER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "CarController.h"

// These UUIDs should be customized for your application
#define SERVO_SERVICE_UUID           "12345678-abcd-1234-abcd-1234567890ab"
#define SET_SERVO_CHAR_UUID          "12345678-abcd-1234-abcd-1234567890ac"

#define MOTOR_SERVICE_UUID           "12345678-abcd-1234-abcd-1234567890bb"
#define SET_MOTOR_ACTION_CHAR_UUID   "12345678-abcd-1234-abcd-1234567890bc"
#define GET_MOTOR_STATE_CHAR_UUID    "12345678-abcd-1234-abcd-1234567890bd"
#define SET_MOTOR_PWM_CHAR_UUID      "12345678-abcd-1234-abcd-1234567890be"

#define ULTRASONIC_SERVICE_UUID      "12345678-abcd-1234-abcd-1234567890cb"
#define GET_ULTRASONIC_DISTANCE_CHAR_UUID "12345678-abcd-1234-abcd-1234567890cc"

class BLEManager {
public:
    BLEManager(CarController& carController);
    void begin();
    
private:
    CarController& carController;
};

#endif
