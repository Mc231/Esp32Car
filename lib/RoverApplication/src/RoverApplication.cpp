#include "RoverApplication.h"

RoverApplication::RoverApplication(const RoverApplicationConfig& cfg)
  : systemMonitor(),
    abstractFs(new FSImpl()),
    config(cfg),
    wiFiConfigManager(*abstractFs),
    setupManager(new WiFiSetupManager(wiFiConfigManager, config.apSsid, config.apPassword)),
    leftMotor(config.leftMotorPin1, config.leftMotorPin2, config.leftMotorPwm),  
    rightMotor(config.rightMotorPin1, config.rightMotorPin2, config.rightMotorPwm), 
    motorControl(leftMotor, rightMotor), 
    ultraSonicManager(config.ultrasonicPin1, config.ultrasonicPin2),
    carController(wiFiConfigManager ,motorControl, ultraSonicManager),
    webServer(carController, config, systemMonitor),
    cameraManager(),
    postSetupBroadcaster(new MDNSBroadcaster(config.mdnsDiscoveryName)),
    isSetupComplete(false)
{}

void RoverApplication::setup() {
  Serial.begin(this->config.serialBaud);
  initializeWiFi();
}

void RoverApplication::loop() {
     if (!isSetupComplete) {
        setupManager->handleClient();
    } else {
       webServer.handleClient();
    }
}

void RoverApplication::initializeWiFi() {
    setupManager->initialize(std::bind(&RoverApplication::setupCompleted, this));
}

void RoverApplication::setupCompleted() {
  setupManager->stopServices();
  Serial.println("Wi-Fi connected. Setup completed!");
  postSetupBroadcaster->begin();
  isSetupComplete = true;

  cameraManager.initialize();

  webServer.begin();
  startCameraServer();
  //ultraSonicManager.initialize();
}