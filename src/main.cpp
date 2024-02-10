#include <Arduino.h>
#include <RoverApplication.h>

RoverApplicationConfig customConfig;
RoverApplication* roverApp = nullptr;


void setup() {;
  customConfig.ultrasonicSensorEnabled = true;
  customConfig.mdnsDiscoveryName = "RoverCar";
  roverApp = new RoverApplication(customConfig);
  roverApp->setup();
}

void loop() {
  if (roverApp != nullptr) {
    roverApp->loop();
  }
}
