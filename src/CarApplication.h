#ifndef CARAPPLICATION_H
#define CARAPPLICATION_H

#include "ServoManager.h"
#include "WiFiManager.h"
#include "HttpManager.h"
#include "MotorControl.h"
#include "ServoControl.h"
#include "UltrasonicManager.h"
#include "CameraManager.h"
#include "CarController.h"
#include "esp_camera.h"
#include "BLEManager.h"
#include "SPIFFSStorageManager.h"
#include "ConfigManager.h"
#include "CarApplicationConfig.h"

void startCameraServer();

class CarApplication {
public:
  CarApplication(const CarApplicationConfig& config);
  void setup();
  void loop();
private:
  ConfigManager configManager;
  WiFiManager wifiManager;  
  ServoManager horizontalServoManager;
  ServoManager verticalServoManager;
  ServoControl servoControl;
  MotorManager leftMotor; 
  MotorManager rightMotor; 
  MotorControl motorControl;
  UltrasonicManager ultraSonicManager;
  CarController carController;
  HttpManager httpManager;
  BLEManager bleManager;
  CameraManager cameraManager;
};

#endif // CARAPPLICATION_H
