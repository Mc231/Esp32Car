#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <Arduino.h>

class OTAManager {
public:
  OTAManager(const char* hostname, const char* password = "");
  void begin();
  void loop();

private:
  const char* hostname;
  const char* password;
  bool started = false;
};

#endif
