#include "OTAManager.h"
#include <ArduinoOTA.h>

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
      Serial.printf("OTA: start updating %s\n", type);
    })
    .onEnd([]() {
      Serial.println("\nOTA: complete, rebooting");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("OTA error[%u]: ", error);
      switch (error) {
        case OTA_AUTH_ERROR:    Serial.println("auth failed"); break;
        case OTA_BEGIN_ERROR:   Serial.println("begin failed"); break;
        case OTA_CONNECT_ERROR: Serial.println("connect failed"); break;
        case OTA_RECEIVE_ERROR: Serial.println("receive failed"); break;
        case OTA_END_ERROR:     Serial.println("end failed"); break;
        default:                Serial.println("unknown"); break;
      }
    });

  ArduinoOTA.begin();
  started = true;
  Serial.printf("OTA: ready as '%s.local'\n", hostname);
}

void OTAManager::loop() {
  if (started) ArduinoOTA.handle();
}
