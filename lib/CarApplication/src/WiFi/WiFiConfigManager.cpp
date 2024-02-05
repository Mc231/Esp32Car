// WiFiConfigManager.cpp
#include "WiFiConfigManager.h"

WiFiConfigManager::WiFiConfigManager(AbstractFS& fs) : fileSystem(fs), isConfigCached(false) {
    fileSystem.begin();
}

WiFiConfigManager::Config WiFiConfigManager::readConfig() {
    if (!isConfigCached) {
        if (fileSystem.exists(WIFI_CONFIG_FILE)) {
            File configFile = fileSystem.open(WIFI_CONFIG_FILE, "r");
            if (configFile) {
                while (configFile.available()) {
                    String line = configFile.readStringUntil('\n');
                    line = trimString(line);

                    if (cachedConfig.ssid.length() == 0) {
                        cachedConfig.ssid = line;
                    } else if (cachedConfig.password.length() == 0) {
                        cachedConfig.password = line;
                    }
                }
                configFile.close();
            }
        }
        isConfigCached = true; // Set the cache flag
    }
    return cachedConfig;
}

void WiFiConfigManager::saveConfig(const String& ssid, const String& password) {
    File configFile = fileSystem.open(WIFI_CONFIG_FILE, "w");
    if (configFile) {
        configFile.println(ssid);
        configFile.println(password);
        configFile.close();
        // Update the cache
        cachedConfig.ssid = ssid;
        cachedConfig.password = password;
        isConfigCached = true;
    }
}

void WiFiConfigManager::clearConfig() {
    if (fileSystem.begin()) {
        if (fileSystem.exists(WIFI_CONFIG_FILE)) {
            fileSystem.remove(WIFI_CONFIG_FILE);
            Serial.println("WI-FI config configuration cleared.");
            isConfigCached = false; // Invalidate the cache
        } else {
            Serial.println("No WI-FI config configuration to clear.");
        }
        fileSystem.end();
    } else {
        Serial.println("Failed to initialize file system for clearing WI-FI config.");
    }
}

String WiFiConfigManager::trimString(const String& str) {
    String trimmed = str;
    trimmed.trim();
    return trimmed;
}
