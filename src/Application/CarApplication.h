#ifndef CARAPPLICATION_H
#define CARAPPLICATION_H

#include "WiFi/WiFiSetupManager.h"
#include "WiFi/WiFiConfigManager.h"
#include "CarWebServer/CarWebServer.h"
#include "Control/MotorControl.h"
#include "Manager/UltrasonicManager.h"
#include "Camera/CameraManager.h"
#include "Controller/CarController.h"
#include "esp_camera.h"
#include "Fs/FSImpl.h"
#include "Monitor/SystemMonitor.h"

void startCameraServer();

class CarApplication {
public:
  CarApplication();
  //CarApplication(const CarApplicationConfig& config, ConfigManager& configMgr);
  void setup();
  void loop();

private:
  SystemMonitor systemMonitor;
  AbstractFS* abstractFs;
  CarApplicationConfig config;
  WiFiConfigManager wiFiConfigManager;
  AbstractWiFiSetupManager* setupManager;  
  MotorManager leftMotor; 
  MotorManager rightMotor; 
  MotorControl motorControl;
  UltrasonicManager ultraSonicManager;
  CarController carController;
  CarWebServer webServer;
  CameraManager cameraManager;

  bool isSetupComplete;

  void setupCompleted();
  void initializeWiFi();
};

#endif // CARAPPLICATION_H
