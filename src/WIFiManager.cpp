#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* ssid, const char* password) : ssid(ssid), password(password) {}

void WiFiManager::connect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

String WiFiManager::getIPAddress() {
  IPAddress ip = WiFi.localIP();
  return ip.toString();
}
