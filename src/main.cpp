#include <Arduino.h>
#include <RoverApplication.h>

// Default config: AP-mode SSID "Rover", no admin auth, MQTT off.
// Override fields here to customize per build:
//
//   RoverApplicationConfig cfg;
//   cfg.adminUser     = "admin";
//   cfg.adminPassword = "hunter2";
//   cfg.mqttEnabled   = true;
//   cfg.mqttHost      = "192.168.1.10";
//   cfg.mqttUser      = "rover";
//   cfg.mqttPassword  = "secret";
//   RoverApplication roverApp(cfg);
//
// MQTT settings are only consulted when the firmware is built with
// -DROVER_FEATURE_MQTT (env: wrover-cam-mqtt). Don't commit real
// secrets — keep your overrides in a local main.cpp or a gitignored
// header.
RoverApplication roverApp;

void setup() {
  roverApp.setup();
}

void loop() {
  roverApp.loop();
}
