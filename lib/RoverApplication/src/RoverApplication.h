#ifndef ROVERAPPLICATION_H
#define ROVERAPPLICATION_H

#include "WiFi/WiFiSetupManager.h"
#include "WiFi/WiFiConfigManager.h"
#include "RoverWebServer/RoverWebServer.h"
#include "Control/MotorControl.h"
#include "Manager/UltrasonicManager.h"
#include "Camera/CameraManager.h"
#include "Controller/RoverController.h"
#include "esp_camera.h"
#include "Fs/FSImpl.h"
#include "Monitor/SystemMonitor.h"
#include "PostSetupBroadcaster/MDNSBroadcaster.h"

void startCameraServer();

class RoverApplication {
public:
  RoverApplication();
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
  UltrasonicManager ultraSonicManager;
  RoverController carController;
  RoverWebServer webServer;
  CameraManager cameraManager;
  PostSetupAvailabilityBroadcaster* postSetupBroadcaster;

  bool isSetupComplete;

  void setupCompleted();
  void initializeWiFi();
};

#endif // ROVERAPPLICATION_H
