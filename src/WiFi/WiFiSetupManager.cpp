// WiFiSetupManager.cpp
#include "WiFiSetupManager.h"
#include "CaptivePortalHTML.h"

const char* ap_ssid = "EspCar";
const char* ap_password = "123456789";

WiFiSetupManager::WiFiSetupManager(WiFiConfigManager& cm) 
    : configManager(cm), webServer(80) {}

void WiFiSetupManager::initialize(SetupCompleteCallback callback) {
    setupCompleteCallback = callback;
    WiFiConfigManager::Config config = configManager.readConfig();
    if (!config.ssid.isEmpty() && !config.password.isEmpty()) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.ssid.c_str(), config.password.c_str());

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.println("Failed to connect. Entering setup mode...");
            startAPMode();
        } else {
            Serial.println("Connected to Wi-Fi.");
            if (setupCompleteCallback) {
                setupCompleteCallback();
            }
        }
    } else {
        Serial.println("Entering setup mode...");
        startAPMode();
    }
}

void WiFiSetupManager::startAPMode() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid, ap_password);

    dnsServer.start(53, "*", WiFi.softAPIP());
    setupCaptivePortal();

    Serial.println("AP Mode and Captive Portal started.");
}

void WiFiSetupManager::setupCaptivePortal() {
    webServer.on("/", HTTP_GET, std::bind(&WiFiSetupManager::handleRoot, this));
    webServer.on("/scan", HTTP_GET, std::bind(&WiFiSetupManager::handleScanNetworks, this));
    webServer.on("/connect", HTTP_POST, std::bind(&WiFiSetupManager::handleConnectToNetwork, this));
    webServer.onNotFound(std::bind(&WiFiSetupManager::handleNotFound, this));
    webServer.begin();
}

void WiFiSetupManager::handleRoot() {
    // Handle the root path for the captive portal
    webServer.send(200, "text/html", captivePortalHTML);
}

void WiFiSetupManager::handleNotFound() {
    // Redirect all requests to the root path
    webServer.sendHeader("Location", "/", true);
    webServer.send(302, "text/plain", "");
}

void WiFiSetupManager::handleClient() {
    dnsServer.processNextRequest();
    webServer.handleClient();
}

void WiFiSetupManager::stopServices() {
    dnsServer.stop();
    webServer.stop();
    Serial.println("HTTP server and DNS server stopped.");
}

void WiFiSetupManager::handleScanNetworks() {
    String json = "[";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        json += "{\"SSID\":\"" + WiFi.SSID(i) + "\",\"RSSI\":" + WiFi.RSSI(i) + "}";
    }
    json += "]";
    webServer.send(200, "application/json", json);
    WiFi.scanDelete();
}

void WiFiSetupManager::handleConnectToNetwork() {
    if (webServer.hasArg("ssid") && webServer.hasArg("password")) {
        String ssid = webServer.arg("ssid");
        String password = webServer.arg("password");

        WiFi.begin(ssid.c_str(), password.c_str());

        int count = 0;
        while (WiFi.status() != WL_CONNECTED && count < 30) {
            delay(1000);
            count++;
            Serial.print(".");
        }
        
    if (WiFi.status() == WL_CONNECTED) {
        webServer.send(200, "text/plain", "Connected to " + ssid);
        configManager.saveConfig(ssid, password);
        delay(1000); 
        ESP.restart(); 
    } else {
        webServer.send(504, "text/plain", "Connection failed");
    }
    } else {
        webServer.send(400, "text/plain", "Missing ssid or password");
    }
}