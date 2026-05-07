#ifndef ROVERAPPLICATION_H
#define ROVERAPPLICATION_H

#include "WiFi/WiFiSetupManager.h"
#include "WiFi/WiFiConfigManager.h"
#include "RoverWebServer/RoverWebServer.h"
#include "RoverWebSocket/RoverWebSocketServer.h"
#include "Serial/RoverSerialServer.h"
#include "EspNow/RoverEspNowServer.h"
#include "Command/CommandDispatcher.h"
#include "Command/TransportRegistry.h"
#include "Control/MotorControl.h"
#include "Manager/DistanceManager.h"
#include "Camera/CameraManager.h"
#include "esp_camera.h"
#include "Controller/RoverController.h"
#include "Fs/FSImpl.h"
#include "Monitor/SystemMonitor.h"
#include "PostSetupBroadcaster/MDNSBroadcaster.h"
#include "Ota/OTAManager.h"
#include "Log/RemoteLogger.h"

#ifdef ROVER_FEATURE_MQTT
#include "Mqtt/RoverMqttClient.h"
#endif

void startCameraServer();

class RoverApplication {
public:
  RoverApplication(const RoverApplicationConfig& cfg = RoverApplicationConfig());
  void setup();
  void loop();

private:
  SystemMonitor systemMonitor;
  AbstractFS* abstractFs;
  RoverApplicationConfig config;
  WiFiConfigManager wiFiConfigManager;
  AbstractWiFiSetupManager* setupManager;
  MotorManager leftMotor;
  MotorManager rightMotor;
  MotorControl motorControl;
  DistanceManager distanceManager;
  RoverController carController;
  CommandDispatcher commandDispatcher;
  TransportRegistry transports;
  RoverWebServer webServer;
  RoverWebSocketServer webSocketServer;
  RoverSerialServer serialTransport;
  RoverEspNowServer espNowTransport;
  CameraManager cameraManager;
  PostSetupAvailabilityBroadcaster* postSetupBroadcaster;
  OTAManager otaManager;

#ifdef ROVER_FEATURE_MQTT
  RoverMqttClient* mqttClient = nullptr;
#endif

  bool isSetupComplete;

  void setupCompleted();
  void initializeWiFi();
  void registerTransports();
};

#endif // ROVERAPPLICATION_H
