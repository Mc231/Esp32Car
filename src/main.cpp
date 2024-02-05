#include <Arduino.h>
#include <CarApplication.h>

CarApplication carApp;

void setup() {;
  carApp.setup();
}

void loop() {
  carApp.loop();
}
