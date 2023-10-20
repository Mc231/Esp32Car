#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
  WiFiManager(const char* ssid, const char* password);
  void connect();
  String getIPAddress();

private:
  const char* ssid;
  const char* password;
};

#endif // WIFIMANAGER_H