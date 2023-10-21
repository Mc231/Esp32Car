#include "WiFiManager.h"

WiFiManager::WiFiManager() {}

void WiFiManager::connect(const std::string& ssid, const std::string& password) {
  this->ssid = ssid;
  this->password = password;
  
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

std::string WiFiManager::getIPAddress() {
  IPAddress ip = WiFi.localIP();
  return ip.toString().c_str();
}
