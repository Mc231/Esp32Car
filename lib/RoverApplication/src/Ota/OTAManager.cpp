#include "OTAManager.h"
#include <ArduinoOTA.h>
#include "Log/RemoteLogger.h"

OTAManager::OTAManager(const char* hostname, const char* password)
  : hostname(hostname), password(password) {}

void OTAManager::begin() {
  ArduinoOTA.setHostname(hostname);
  if (password && password[0] != '\0') {
    ArduinoOTA.setPassword(password);
  }

  ArduinoOTA
    .onStart([]() {
      const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
      Log.printf("OTA: start updating %s\n", type);
    })
    .onEnd([]() {
      Log.println("\nOTA: complete, rebooting");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Log.printf("OTA: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Log.printf("OTA error[%u]: ", error);
      switch (error) {
        case OTA_AUTH_ERROR:    Log.println("auth failed"); break;
        case OTA_BEGIN_ERROR:   Log.println("begin failed"); break;
        case OTA_CONNECT_ERROR: Log.println("connect failed"); break;
        case OTA_RECEIVE_ERROR: Log.println("receive failed"); break;
        case OTA_END_ERROR:     Log.println("end failed"); break;
        default:                Log.println("unknown"); break;
      }
    });

  ArduinoOTA.begin();
  started = true;
  Log.printf("OTA: ready as '%s.local'\n", hostname);
}

void OTAManager::loop() {
  if (started) ArduinoOTA.handle();
}
