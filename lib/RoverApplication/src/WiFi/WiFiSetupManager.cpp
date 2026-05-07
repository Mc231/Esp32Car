// WiFiSetupManager.cpp
#include "WiFiSetupManager.h"
#include "CaptivePortalHTML.h"
#include "Log/RemoteLogger.h"


WiFiSetupManager::WiFiSetupManager(WiFiConfigManager& cm,
                                   RuntimeConfigManager& rc,
                                   const char *apSsid, const char *apPassword)
    : configManager(cm), runtimeConfig(rc), apSsid(apSsid), apPassword(apPassword), webServer(80) {}

void WiFiSetupManager::initialize(SetupCompleteCallback callback) {
    setupCompleteCallback = callback;
    WiFiConfigManager::Config config = configManager.readConfig();
    if (!config.ssid.isEmpty() && !config.password.isEmpty()) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.ssid.c_str(), config.password.c_str());

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Log.println("Failed to connect. Entering setup mode...");
            startAPMode();
        } else {
            Log.println("Connected to Wi-Fi.");
            if (setupCompleteCallback) {
                setupCompleteCallback();
            }
        }
    } else {
        Log.println("Entering setup mode...");
        startAPMode();
    }
}

void WiFiSetupManager::startAPMode() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(this->apSsid, this->apPassword);

    dnsServer.start(53, "*", WiFi.softAPIP());
    setupCaptivePortal();

    Log.println("AP Mode and Captive Portal started.");
}

void WiFiSetupManager::setupCaptivePortal() {
    webServer.on("/", HTTP_GET, std::bind(&WiFiSetupManager::handleRoot, this));
    webServer.on("/scan", HTTP_GET, std::bind(&WiFiSetupManager::handleScanNetworks, this));
    webServer.on("/connect", HTTP_POST, std::bind(&WiFiSetupManager::handleConnectToNetwork, this));
    webServer.on("/advanced", HTTP_GET, std::bind(&WiFiSetupManager::handleGetAdvanced, this));
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
    Log.println("HTTP server and DNS server stopped.");
}

void WiFiSetupManager::handleScanNetworks() {
    WiFi.scanDelete();
    WiFi.disconnect(false, true);
    delay(100);

    int n = WiFi.scanNetworks(false, true);
    Log.printf("scanNetworks found %d\n", n);

    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        String ssid = WiFi.SSID(i);
        ssid.replace("\"", "\\\"");
        json += "{\"SSID\":\"" + ssid + "\",\"RSSI\":" + WiFi.RSSI(i) + "}";
    }
    json += "]";
    webServer.send(200, "application/json", json);
    WiFi.scanDelete();
}

void WiFiSetupManager::handleGetAdvanced() {
    // Return current runtime config so the form can pre-fill.
    const auto& cfg = runtimeConfig.read();
    String json = "{";
    json += "\"mqtt\":{";
    json += "\"enabled\":";   json += (cfg.mqttEnabled ? "true" : "false");
    json += ",\"host\":\"";   json += cfg.mqttHost; json += "\"";
    json += ",\"port\":";     json += cfg.mqttPort;
    json += ",\"user\":\"";   json += cfg.mqttUser; json += "\"";
    // Don't echo password back — return empty string for the form.
    json += ",\"password\":\"\"";
    json += ",\"clientId\":\""; json += cfg.mqttClientId; json += "\"";
    json += ",\"topicPrefix\":\""; json += cfg.mqttTopicPrefix; json += "\"";
    json += "}}";
    webServer.send(200, "application/json", json);
}

void WiFiSetupManager::saveAdvancedFromForm() {
    auto cfg = runtimeConfig.read();   // start from existing
    bool changed = false;
    auto strArg = [&](const char* k) -> String {
        return webServer.hasArg(k) ? webServer.arg(k) : String("");
    };
    auto boolArg = [&](const char* k) -> bool {
        if (!webServer.hasArg(k)) return false;
        String v = webServer.arg(k); v.toLowerCase();
        return v == "1" || v == "true" || v == "on" || v == "yes";
    };

    if (webServer.hasArg("mqtt_enabled")) { cfg.mqttEnabled = boolArg("mqtt_enabled"); changed = true; }
    if (webServer.hasArg("mqtt_host"))    { cfg.mqttHost    = strArg("mqtt_host"); changed = true; }
    if (webServer.hasArg("mqtt_port"))    { cfg.mqttPort    = strArg("mqtt_port").toInt(); changed = true; }
    if (webServer.hasArg("mqtt_user"))    { cfg.mqttUser    = strArg("mqtt_user"); changed = true; }
    if (webServer.hasArg("mqtt_password")) {
        // Empty string means "leave existing password unchanged".
        String p = strArg("mqtt_password");
        if (p.length() > 0) { cfg.mqttPassword = p; changed = true; }
    }
    if (webServer.hasArg("mqtt_client_id"))    { cfg.mqttClientId    = strArg("mqtt_client_id"); changed = true; }
    if (webServer.hasArg("mqtt_topic_prefix")) { cfg.mqttTopicPrefix = strArg("mqtt_topic_prefix"); changed = true; }

    if (changed) {
        runtimeConfig.save(cfg);
        Log.println("Advanced config saved.");
    }
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
            Log.print(".");
        }

    if (WiFi.status() == WL_CONNECTED) {
        webServer.send(200, "text/plain", "Connected to " + ssid);
        configManager.saveConfig(ssid, password);
        // Persist any advanced settings the user filled in alongside Wi-Fi.
        saveAdvancedFromForm();
        delay(1000);
        ESP.restart();
    } else {
        webServer.send(504, "text/plain", "Connection failed");
    }
    } else {
        webServer.send(400, "text/plain", "Missing ssid or password");
    }
}
