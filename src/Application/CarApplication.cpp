#include "CarApplication.h"

CarApplication::CarApplication()
  : systemMonitor(),
    abstractFs(new FSImpl()),
    config(CarApplicationConfig()),
    wiFiConfigManager(*abstractFs),
    setupManager(new WiFiSetupManager(wiFiConfigManager)),
    leftMotor(config.leftMotorPin1, config.leftMotorPin2, config.leftMotorPwm),  
    rightMotor(config.rightMotorPin1, config.rightMotorPin2, config.rightMotorPwm), 
    motorControl(leftMotor, rightMotor), 
    ultraSonicManager(config.ultrasonicPin1, config.ultrasonicPin2),
    carController(wiFiConfigManager ,motorControl, ultraSonicManager),
    webServer(carController, config, systemMonitor),
    cameraManager(),
    postSetupBroadcaster(new MDNSBroadcaster()),
    isSetupComplete(false)
{}

void CarApplication::setup() {
  Serial.begin(115200);
  initializeWiFi();
}

void CarApplication::loop() {
     if (!isSetupComplete) {
        setupManager->handleClient();
    } else {
       webServer.handleClient();
    }
}

void CarApplication::initializeWiFi() {
    setupManager->initialize(std::bind(&CarApplication::setupCompleted, this));
}

void CarApplication::setupCompleted() {
  setupManager->stopServices();
  Serial.println("Wi-Fi connected. Setup completed!");
  postSetupBroadcaster->begin();
  isSetupComplete = true;

  cameraManager.initialize();

  webServer.begin();
  startCameraServer();
  //ultraSonicManager.initialize();
  
}