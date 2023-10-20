#include <Arduino.h>

struct CarApplicationConfig {
    const char* ssid;
    const char* ssidPassword;
    int horizontalServoPin;
    int verticalServoPin;
    int leftMotorPin1;
    int leftMotorPin2;
    int leftMotorPwm;
    int rightMotorPin1;
    int rightMotorPin2;
    int rightMotorPwm;
    int ultrasonicPin1;
    int ultrasonicPin2;
};