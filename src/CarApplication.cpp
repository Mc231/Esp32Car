#include "CarApplication.h"

CarApplication::CarApplication(const CarApplicationConfig& config)
  : configManager(configManager),
    wifiManager(config.ssid, config.ssidPassword),
    horizontalServoManager(config.horizontalServoPin),
    verticalServoManager(config.verticalServoPin),
    servoControl(horizontalServoManager, verticalServoManager),  
    leftMotor(config.leftMotorPin1, config.leftMotorPin2, config.leftMotorPwm),  
    rightMotor(config.rightMotorPin1, config.rightMotorPin2, config.rightMotorPwm), 
    motorControl(leftMotor, rightMotor), 
    ultraSonicManager(config.ultrasonicPin1, config.ultrasonicPin2),
    carController(servoControl, motorControl, ultraSonicManager),
    httpManager(carController, configManager),
    bleManager(carController),
    cameraManager()
{}

void CarApplication::setup() {
  Serial.println("Setup");
  cameraManager.initialize();
  // Connect to WiFi
  wifiManager.connect();
    // Log IP address
  Serial.print("IP Address: ");
  Serial.println(wifiManager.getIPAddress());
  //bleManager.begin();
  httpManager.begin();
  startCameraServer();
  ultraSonicManager.initialize();
}

void CarApplication::loop() {
  // Future logic can go here
  httpManager.handleClient();
}
