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
    webSocketServer(carController, config, systemMonitor),
    cameraManager(),
    postSetupBroadcaster(new MDNSBroadcaster(config.mdnsDiscoveryName, {
        {"http",       "tcp", 80},
        {"rover-ctrl", "tcp", static_cast<uint16_t>(config.webServerPort)},
        {"rover-ws",   "tcp", static_cast<uint16_t>(config.webSocketPort)},
        {"arduino",    "tcp", 3232}                                            // ArduinoOTA
    })),
    otaManager(config.mdnsDiscoveryName, config.adminPassword),
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
       webSocketServer.loop();
       otaManager.loop();
       if (this->config.ultrasonicSensorEnabled) {
         ultraSonicManager.update();
       }
    }
}

void RoverApplication::initializeWiFi() {
    setupManager->initialize(std::bind(&RoverApplication::setupCompleted, this));
}

void RoverApplication::setupCompleted() {
  setupManager->stopServices();
  Serial.print("Wi-Fi connected. IP: ");
  Serial.print(WiFi.localIP());
  Serial.print("  mDNS: http://");
  Serial.print(this->config.mdnsDiscoveryName);
  Serial.print(".local:");
  Serial.println(this->config.webServerPort);
  postSetupBroadcaster->begin();
  isSetupComplete = true;

  cameraManager.initialize();

  webServer.begin();
  webSocketServer.begin();
  startCameraServer();
  otaManager.begin();
  if (this->config.ultrasonicSensorEnabled)
  {
    ultraSonicManager.initialize();
  }
}