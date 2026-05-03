
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
    int distanceSensorPin;
    bool distanceSensorEnabled;
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
          // PSRAM eats IO16/17 on WROVER-E. IO33 is ADC1_CH5 — analog-capable
          // and ADC1 plays nicely with Wi-Fi. Sharp GP2Y0A21 IR distance
          // sensor's analog output lands here directly (no resistors needed,
          // 3.1V max output fits within the ESP32 ADC range).
          distanceSensorPin(33),
          distanceSensorEnabled(true),
#else
          distanceSensorPin(16),
          distanceSensorEnabled(false),
#endif
          adminUser(""),
          adminPassword(""),
          deadmanTimeoutMs(1500) { }
};

#endif // ROVERAPPLICATIONCONFIG_H
