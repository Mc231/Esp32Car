#include "CarApplication.h"

CarApplication::CarApplication(const CarApplicationConfig& config, ConfigManager& configMgr)
  : config(config),
    configManager(configMgr),
    wifiManager(),
    leftMotor(config.leftMotorPin1, config.leftMotorPin2, config.leftMotorPwm),  
    rightMotor(config.rightMotorPin1, config.rightMotorPin2, config.rightMotorPwm), 
    motorControl(leftMotor, rightMotor), 
    ultraSonicManager(config.ultrasonicPin1, config.ultrasonicPin2),
    carController(motorControl, ultraSonicManager),
    httpManager(carController, configManager),
    cameraManager()
{}

void CarApplication::setup() {
  Serial.println("Setup");
  cameraManager.initialize();
  // Connect to WiFi
  wifiManager.connect(config.ssid, config.ssidPassword);
  // Log IP address
  Serial.print("IP Address: ");
  Serial.println(wifiManager.getIPAddress().c_str());
  httpManager.begin();
  startCameraServer();
  ultraSonicManager.initialize();
}

void CarApplication::loop() {
  // Future logic can go here
  httpManager.handleClient();
}
