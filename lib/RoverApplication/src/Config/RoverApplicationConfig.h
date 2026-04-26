
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
    // HTTP basic auth for the control panel (port webServerPort).
    // Both empty = auth disabled.
    const char* adminUser;
    const char* adminPassword;
    // Deadman: auto-stop motors if no driving command arrives within this window.
    // 0 = disabled. Default: 1500 ms.
    unsigned long deadmanTimeoutMs;

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
#ifdef ROVER_BOARD_WROVER_CAM
          // IO4 is camera Y2 on WROVER_KIT pin map; use IO32 for L298N ENA.
          leftMotorPwm(32),
#else
          leftMotorPwm(4),
#endif
          rightMotorPin1(15),
          rightMotorPin2(13),
          rightMotorPwm(12),
#ifdef ROVER_BOARD_WROVER_CAM
          // IO16/17 are consumed by PSRAM on real WROVER-E. Disable until
          // single-pin HC-SR04 refactor lands and a free pin is chosen.
          ultrasonicPin1(-1),
#else
          ultrasonicPin1(16),
#endif
          ultrasonicPin2(33),
          ultrasonicSensorEnabled(false),
          adminUser(""),
          adminPassword(""),
          deadmanTimeoutMs(1500) { }
};

#endif // ROVERAPPLICATIONCONFIG_H