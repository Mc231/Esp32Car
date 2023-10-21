#ifndef CARAPPLICATION_H
#define CARAPPLICATION_H

#include "WiFi/WiFiManager.h"
#include "CarWebServer/CarWebServer.h"
#include "Control/MotorControl.h"
#include "Manager/UltrasonicManager.h"
#include "Camera/CameraManager.h"
#include "Controller/CarController.h"
#include "esp_camera.h"
#include "Storage/SPIFFSStorageManager.h"
#include "Config/ConfigManager.h"

void startCameraServer();

class CarApplication {
public:
  CarApplication(const CarApplicationConfig& config, ConfigManager& configMgr);
  void setup();
  void loop();

private:
  CarApplicationConfig config;
  ConfigManager& configManager;
  WiFiManager wifiManager;  
  MotorManager leftMotor; 
  MotorManager rightMotor; 
  MotorControl motorControl;
  UltrasonicManager ultraSonicManager;
  CarController carController;
  CarWebServer webServer;
  CameraManager cameraManager;
};

#endif // CARAPPLICATION_H
