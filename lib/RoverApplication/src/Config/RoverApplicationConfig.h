
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
    // Single GPIO drives HC-SR04 in single-pin mode (TRIG+ECHO joined externally
    // through a 1k resistor, line tapped via 1k/2k divider down to 3.3V).
    int ultrasonicPin;
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
          // PSRAM eats IO16/17 on WROVER-E. IO33 is the only free non-strapping
          // pin on the side header — single-pin HC-SR04 lives there.
          // Disabled by default: the standard 5V HC-SR04 with push-pull ECHO
          // doesn't work in single-pin mode (ECHO output fights the trigger
          // pulse). Re-enable once an HC-SR04P (3.3V variant) is wired in.
          ultrasonicPin(33),
          ultrasonicSensorEnabled(false),
#else
          ultrasonicPin(16),
          ultrasonicSensorEnabled(false),
#endif
          adminUser(""),
          adminPassword(""),
          deadmanTimeoutMs(1500) { }
};

#endif // ROVERAPPLICATIONCONFIG_H
