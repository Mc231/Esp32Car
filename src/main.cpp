#include <Arduino.h>
#include "AppInitializer.h"
#include "CarApplication.h"

AppInitializer appInit;
CarApplication* carApp = nullptr;

void setup() {
    Serial.begin(115200);
    Serial.print("Free heap memory: ");
    Serial.println(ESP.getFreeHeap());
  carApp = appInit.initializeApplication("/config.cfg");
 // carApp->setup();
}

void loop() {
  if (carApp) {
   // carApp->loop();
  }
}
