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
    distanceManager(config.distanceSensorPin),
    carController(wiFiConfigManager, motorControl, distanceManager),
    commandDispatcher(carController, config, systemMonitor),
    transports(),
    webServer(config, commandDispatcher),
    webSocketServer(commandDispatcher, config),
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
       transports.loopAll();
       otaManager.loop();
       Log.loop();
       carController.tickDeadman();
#ifdef ROVER_FEATURE_DISTANCE
       distanceManager.update();
#endif
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
  Log.printf("Wi-Fi connected. IP: %s  mDNS: %s.local\n",
             WiFi.localIP().toString().c_str(),
             this->config.mdnsDiscoveryName);
  Log.printf("API:    POST http://%s.local:%d/api/cmd\n",
             this->config.mdnsDiscoveryName, this->config.webServerPort);
  Log.printf("WS:     ws://%s.local:%d/\n",
             this->config.mdnsDiscoveryName, this->config.webSocketPort);
  Log.printf("Camera: http://%s.local:81/stream\n", this->config.mdnsDiscoveryName);
  Log.printf("OTA:    http://%s.local:%d/ota\n",
             this->config.mdnsDiscoveryName, this->config.webServerPort);
  Log.printf("Logs:   http://%s.local:%d/logs\n",
             this->config.mdnsDiscoveryName, this->config.webServerPort);
  Log.printf("Telnet: nc %s 23\n", WiFi.localIP().toString().c_str());
  postSetupBroadcaster->begin();
  isSetupComplete = true;

  startCameraServer();
  otaManager.begin();
  carController.setDeadmanTimeout(this->config.deadmanTimeoutMs);
#ifdef ROVER_FEATURE_DISTANCE
  distanceManager.initialize();
#endif

  registerTransports();
}

void RoverApplication::registerTransports() {
  // Always-on transports first — they're the bootstrap channels (you
  // need them to flip the others via `set_transport`).
  transports.add(&webServer);
  transports.add(&webSocketServer);

#ifdef ROVER_FEATURE_MQTT
  RoverMqttClient::Config mc;
  mc.host        = config.mqttHost ? std::string(config.mqttHost) : std::string();
  mc.port        = static_cast<uint16_t>(config.mqttPort);
  mc.user        = config.mqttUser ? std::string(config.mqttUser) : std::string();
  mc.password    = config.mqttPassword ? std::string(config.mqttPassword) : std::string();
  mc.clientId    = config.mqttClientId ? std::string(config.mqttClientId) : std::string();
  mc.topicPrefix = config.mqttTopicPrefix ? std::string(config.mqttTopicPrefix) : std::string();
  mqttClient = new RoverMqttClient(commandDispatcher, mc);
  transports.add(mqttClient);
#endif

  // Wire the registry into the dispatcher so the `transports` and
  // `set_transport` commands can find + toggle transports.
  commandDispatcher.setTransportRegistry(&transports);

  // Start always-on transports unconditionally; MQTT only if enabled in
  // the build-time config and the host string is non-empty.
  webServer.begin();
  webSocketServer.begin();
#ifdef ROVER_FEATURE_MQTT
  if (config.mqttEnabled && config.mqttHost && config.mqttHost[0] != '\0') {
    mqttClient->begin();
  } else {
    Log.println("[mqtt] disabled (config.mqttEnabled false or host empty)");
  }
#endif
}
