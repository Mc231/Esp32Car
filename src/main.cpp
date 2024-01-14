#include <Arduino.h>
#include "Application/CarApplication.h"

CarApplication carApp;

void setup() {;
  carApp.setup();
}

void loop() {
  carApp.loop();
}
