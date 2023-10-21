#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <string>

class WiFiManager {
public:
  WiFiManager();
  void connect(const std::string& ssid, const std::string& password);
  std::string getIPAddress();

private:
  std::string ssid;
  std::string password;
};

#endif // WIFIMANAGER_H
