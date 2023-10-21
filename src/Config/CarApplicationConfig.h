#include <string>

struct CarApplicationConfig {
    std::string ssid;
    std::string ssidPassword;
    int leftMotorPin1;
    int leftMotorPin2;
    int leftMotorPwm;
    int rightMotorPin1;
    int rightMotorPin2;
    int rightMotorPwm;
    int ultrasonicPin1;
    int ultrasonicPin2;
};