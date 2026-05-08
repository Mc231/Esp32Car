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
    serialTransport(commandDispatcher),
    espNowTransport(commandDispatcher),
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
  // Bring Log up briefly so the boot banner reaches any client that
  // happens to be watching telnet — but it'll be stopped below if the
  // boot config says telnet is off, leaving Serial as the only sink.
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
  Log.printf("Telnet: nc %s 23%s\n",
             WiFi.localIP().toString().c_str(),
             config.telnetEnabledAtBoot ? "" : "  (off at boot — toggle via set_transport)");
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
  transports.add(&serialTransport);
  transports.add(&espNowTransport);
  // Telnet (port 23) is the global Log server — already started in
  // setupCompleted via Log.begin(). Wire the dispatcher so the read
  // path turns into command dispatch, then register it so set_transport
  // can toggle it like any other.
  Log.setDispatcher(&commandDispatcher);
  transports.add(&Log);

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

  // Start each transport only if its boot flag is set in the config.
  // Defaults: HTTP + WS only — keeps idle current low on battery.
  // The remaining transports are still REGISTERED above, so the user
  // can flip them on at runtime via the `set_transport` command (or
  // the web UI's transports panel).
  if (config.httpEnabledAtBoot)      webServer.begin();
  else                                Log.println("[http] off at boot (config)");
  if (config.webSocketEnabledAtBoot) webSocketServer.begin();
  else                                Log.println("[ws] off at boot (config)");
  if (config.serialTransportEnabledAtBoot) serialTransport.begin();
  else                                      Log.println("[serial] off at boot (config)");
  if (config.espNowEnabledAtBoot)    espNowTransport.begin();
  else                                Log.println("[espnow] off at boot (config)");

  // Telnet is special — Log.begin() above started its TCP server so the
  // boot banner could reach any connected client. If the boot config
  // says telnet is off, stop that TCP listener now. Serial output keeps
  // working either way (RemoteLogger writes Serial unconditionally).
  if (!config.telnetEnabledAtBoot) {
    Log.println("[telnet] off at boot (config) — stopping TCP server");
    Log.stop();
  }

#ifdef ROVER_FEATURE_MQTT
  if (config.mqttEnabled && config.mqttHost && config.mqttHost[0] != '\0') {
    mqttClient->begin();
  } else {
    Log.println("[mqtt] disabled (config.mqttEnabled false or host empty)");
  }
#endif
}
