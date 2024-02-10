#include <Arduino.h>
#include <RoverApplication.h>

RoverApplication roverApp;

void setup() {;
  roverApp.setup();
}

void loop() {
  roverApp.loop();
}
