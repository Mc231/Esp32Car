#include <Arduino.h>
#include "Application/AppInitializer.h"
#include "Application/CarApplication.h"

#define CONFIG_FILE_PATH "/config.cfg"

AppInitializer appInit(CONFIG_FILE_PATH);
CarApplication* carApp = nullptr;

void setup() {
  carApp = appInit.initializeApplication();
  if (carApp) {
    carApp->setup();
  }
}

void loop() {
  if (carApp) {
    carApp->loop();
  }
}
