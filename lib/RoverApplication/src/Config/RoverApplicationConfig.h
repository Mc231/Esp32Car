#ifndef ROVERAPPLICATIONCONFIG_H
#define ROVERAPPLICATIONCONFIG_H

struct RoverApplicationConfig {
    const char* apSsid;
    const char* apPassword;
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
    // Sharp GP2Y0A21 IR distance sensor (analog out). Compiled only when
    // ROVER_FEATURE_DISTANCE is defined; the pin is read from this field.
    int distanceSensorPin;
    // HTTP basic auth for the control HTTP server (POST /api/cmd, /ota,
    // /logs). Both empty = auth disabled.
    const char* adminUser;
    const char* adminPassword;
    // Deadman: auto-stop motors if no driving command arrives within this window.
    // 0 = disabled. Default: 1500 ms.
    unsigned long deadmanTimeoutMs;

    // Per-transport "start at boot" toggles. Every transport is always
    // REGISTERED (so it shows up in `transports` and can be flipped on
    // via `set_transport`); these flags only decide which begin() at
    // boot. Defaults: HTTP + WS only — minimises radio/CPU draw on
    // battery. Toggle the others on at runtime via the web UI when
    // needed.
    bool httpEnabledAtBoot;
    bool webSocketEnabledAtBoot;
    bool serialTransportEnabledAtBoot;
    bool espNowEnabledAtBoot;
    bool telnetEnabledAtBoot;

    // MQTT bridge settings — only consulted when the firmware is built
    // with -DROVER_FEATURE_MQTT. host empty → MQTT stays off even if
    // mqttEnabled is true. Override per-rover from main.cpp.
    bool        mqttEnabled;
    const char* mqttHost;
    int         mqttPort;
    const char* mqttUser;
    const char* mqttPassword;
    const char* mqttClientId;     // empty → derived from MAC at boot
    const char* mqttTopicPrefix;  // empty → "rover"

     RoverApplicationConfig()
        : apSsid("Rover"),
          apPassword("123456789"),
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
#else
          distanceSensorPin(16),
#endif
          adminUser(""),
          adminPassword(""),
          deadmanTimeoutMs(1500),
          httpEnabledAtBoot(true),
          webSocketEnabledAtBoot(true),
          serialTransportEnabledAtBoot(false),
          espNowEnabledAtBoot(false),
          telnetEnabledAtBoot(false),
          mqttEnabled(false),
          mqttHost(""),
          mqttPort(1883),
          mqttUser(""),
          mqttPassword(""),
          mqttClientId(""),
          mqttTopicPrefix("") { }
};

#endif // ROVERAPPLICATIONCONFIG_H
