#include "RoverApplication.h"

// Global logger — Serial + telnet on port 23 once Wi-Fi is up.
RemoteLogger Log(23);

RoverApplication::RoverApplication(const RoverApplicationConfig& cfg)
  : systemMonitor(),
    abstractFs(new FSImpl()),
    config(cfg),
    wiFiConfigManager(*abstractFs),
    runtimeConfig(*abstractFs),
    setupManager(new WiFiSetupManager(wiFiConfigManager, runtimeConfig, config.apSsid, config.apPassword)),
    // LEDC channels 4 and 5 — chosen to avoid esp_camera's XCLK on channel 0.
    leftMotor(config.leftMotorPin1, config.leftMotorPin2, config.leftMotorPwm, 4),
    rightMotor(config.rightMotorPin1, config.rightMotorPin2, config.rightMotorPwm, 5),
    motorControl(leftMotor, rightMotor),
    distanceManager(config.distanceSensorPin),
    carController(wiFiConfigManager, motorControl, distanceManager),
    commandDispatcher(carController, config, systemMonitor),
    transports(),
    webServer(config, commandDispatcher),
    webSocketServer(commandDispatcher, config),
#ifndef ROVER_NO_CAMERA
    cameraManager(),
#endif
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
#ifndef ROVER_NO_CAMERA
  // Camera before motors — both use LEDC, camera owns channel 0 for XCLK,
  // motors get explicit channels 4/5 in MotorManager so there's no clash.
  cameraManager.initialize();
#endif
  motorControl.begin();
  initializeWiFi();
}

void RoverApplication::loop() {
     if (!isSetupComplete) {
        setupManager->handleClient();
    } else {
       transports.loopAll();
       otaManager.loop();
       Log.loop();
       carController.tickDeadman();
       if (this->config.distanceSensorEnabled) {
         distanceManager.update();
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

#ifndef ROVER_NO_CAMERA
  startCameraServer();
#endif
  otaManager.begin();
  carController.setDeadmanTimeout(this->config.deadmanTimeoutMs);
  if (this->config.distanceSensorEnabled)
  {
    distanceManager.initialize();
  }

  registerTransports();
}

void RoverApplication::registerTransports() {
  // Always-on transports first — they're the bootstrap channels (you
  // need them to flip the others via `set_transport`).
  transports.add(&webServer);
  transports.add(&webSocketServer);

  const auto& rc = runtimeConfig.read();

#ifdef ROVER_FEATURE_BLE
  std::string bleName = rc.bleDeviceName.length() > 0
                          ? std::string(rc.bleDeviceName.c_str())
                          : std::string(config.mdnsDiscoveryName);
  std::string blePin = rc.blePin.length() > 0
                         ? std::string(rc.blePin.c_str())
                         : std::string("123456");
  bleServer = new RoverBLEServer(commandDispatcher, bleName, blePin);
  transports.add(bleServer);
#endif

#ifdef ROVER_FEATURE_MQTT
  RoverMqttClient::Config mc;
  mc.host        = std::string(rc.mqttHost.c_str());
  mc.port        = static_cast<uint16_t>(rc.mqttPort);
  mc.user        = std::string(rc.mqttUser.c_str());
  mc.password    = std::string(rc.mqttPassword.c_str());
  mc.clientId    = std::string(rc.mqttClientId.c_str());
  mc.topicPrefix = std::string(rc.mqttTopicPrefix.c_str());
  mqttClient = new RoverMqttClient(commandDispatcher, mc);
  transports.add(mqttClient);
#endif

  // Wire the registry + runtime config into the dispatcher so the
  // `transports` and `set_transport` commands can find/toggle/persist.
  commandDispatcher.setTransportRegistry(&transports);
  commandDispatcher.setRuntimeConfig(&runtimeConfig);

  // Start always-on transports unconditionally; optional ones only if
  // their runtime flag is set.
  webServer.begin();
  webSocketServer.begin();
#ifdef ROVER_FEATURE_BLE
  if (rc.bleEnabled) bleServer->begin();
  else Log.println("[ble] disabled in runtime config");
#endif
#ifdef ROVER_FEATURE_MQTT
  if (rc.mqttEnabled && rc.mqttHost.length() > 0) mqttClient->begin();
  else Log.println("[mqtt] disabled in runtime config");
#endif
}
