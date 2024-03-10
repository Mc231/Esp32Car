
#ifndef ROVERAPPLICATIONCONFIG_H
#define ROVERAPPLICATIONCONFIG_H

struct RoverApplicationConfig {
    const char* apSsid;
    const char* apPassword;
    int setupServerPort;
    int webServerPort;
    int webSocketPort;
    const char* mdnsDiscoveryName;
    int serialBaud;
    int leftMotorPin1;
    int leftMotorPin2;
    int leftMotorPwm;
    int rightMotorPin1;
    int rightMotorPin2;
    int rightMotorPwm;
    int ultrasonicPin1;
    int ultrasonicPin2;
    bool ultrasonicSensorEnabled;

     RoverApplicationConfig() 
        : apSsid("Rover"),
          apPassword("123456789"), 
          setupServerPort(80),
          webServerPort(32231),
          webSocketPort(32232),
          mdnsDiscoveryName("Rover"),
          serialBaud(115200), 
          leftMotorPin1(2), 
          leftMotorPin2(14),
          leftMotorPwm(4),
          rightMotorPin1(15),
          rightMotorPin2(13),
          rightMotorPwm(12),
          ultrasonicPin1(3),
          ultrasonicPin2(1),
          ultrasonicSensorEnabled(true) { }
};

#endif // ROVERAPPLICATIONCONFIG_H