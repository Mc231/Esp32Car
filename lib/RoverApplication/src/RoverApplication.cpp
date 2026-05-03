#include "RoverApplication.h"

// Global logger — Serial + telnet on port 23 once Wi-Fi is up.
RemoteLogger Log(23);

RoverApplication::RoverApplication(const RoverApplicationConfig& cfg)
  : systemMonitor(),
    abstractFs(new FSImpl()),
    config(cfg),
    wiFiConfigManager(*abstractFs),
    setupManager(new WiFiSetupManager(wiFiConfigManager, config.apSsid, config.apPassword)),
    // LEDC channels 4 and 5 — chosen to avoid esp_camera's XCLK on channel 0.
    leftMotor(config.leftMotorPin1, config.leftMotorPin2, config.leftMotorPwm, 4),
    rightMotor(config.rightMotorPin1, config.rightMotorPin2, config.rightMotorPwm, 5),
    motorControl(leftMotor, rightMotor), 
    ultraSonicManager(config.ultrasonicPin),
    carController(wiFiConfigManager ,motorControl, ultraSonicManager),
    webServer(carController, config, systemMonitor),
    webSocketServer(carController, config, systemMonitor),
    cameraManager(),
    postSetupBroadcaster(new MDNSBroadcaster(config.mdnsDiscoveryName, {
        {"http",       "tcp", 80},
        {"rover-ctrl", "tcp", static_cast<uint16_t>(config.webServerPort)},
        {"rover-ws",   "tcp", static_cast<uint16_t>(config.webSocketPort)},
        // _arduino._tcp is registered by ArduinoOTA itself with required TXT records
        {"telnet",     "tcp", 23}                                              // remote serial log
    })),
    otaManager(config.mdnsDiscoveryName, config.adminPassword),
    isSetupComplete(false)
{}

void RoverApplication::setup() {
  Serial.begin(this->config.serialBaud);
  Log.printf("\nRover firmware build %s %s\n", __DATE__, __TIME__);
  // Camera before motors — both use LEDC, camera owns channel 0 for XCLK,
  // motors get explicit channels 4/5 in MotorManager so there's no clash.
  cameraManager.initialize();
  motorControl.begin();
  initializeWiFi();
}

void RoverApplication::loop() {
     if (!isSetupComplete) {
        setupManager->handleClient();
    } else {
       webServer.handleClient();
       webSocketServer.loop();
       otaManager.loop();
       Log.loop();
       carController.tickDeadman();
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
  // Wi-Fi modem sleep starves the camera I2S DMA — fb_get returns NULL
  // ("Camera capture failed") during MJPEG streaming. Trade: a bit more
  // current, but required for stable streaming.
  WiFi.setSleep(false);
  Log.begin();
  Log.printf("Wi-Fi connected. IP: %s  mDNS: http://%s.local:%d\n",
             WiFi.localIP().toString().c_str(),
             this->config.mdnsDiscoveryName,
             this->config.webServerPort);
  Log.printf("Telnet log: nc %s 23\n", WiFi.localIP().toString().c_str());
  postSetupBroadcaster->begin();
  isSetupComplete = true;

  webServer.begin();
  webSocketServer.begin();
  startCameraServer();
  otaManager.begin();
  carController.setDeadmanTimeout(this->config.deadmanTimeoutMs);
  if (this->config.ultrasonicSensorEnabled)
  {
    ultraSonicManager.initialize();
  }
}