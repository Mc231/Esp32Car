// WiFiConfigManager.cpp
#include "WiFiConfigManager.h"
#include "Log/RemoteLogger.h"

WiFiConfigManager::WiFiConfigManager(AbstractFS& fs) : fileSystem(fs), isConfigCached(false), fsMounted(false) {
    // FS mount deferred to ensureMounted(): SPIFFS.begin(true) needs FreeRTOS,
    // but this constructor runs at C++ static-init (before setup()).
}

void WiFiConfigManager::ensureMounted() {
    if (!fsMounted) {
        fsMounted = fileSystem.begin();
        if (!fsMounted) {
            Log.println("WiFiConfigManager: filesystem mount failed");
        }
    }
}

WiFiConfigManager::Config WiFiConfigManager::readConfig() {
    ensureMounted();
    if (!isConfigCached) {
        if (fsMounted && fileSystem.exists(WIFI_CONFIG_FILE)) {
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
    ensureMounted();
    if (!fsMounted) return;
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
    ensureMounted();
    if (!fsMounted) {
        Log.println("Failed to initialize file system for clearing WI-FI config.");
        return;
    }
    if (fileSystem.exists(WIFI_CONFIG_FILE)) {
        fileSystem.remove(WIFI_CONFIG_FILE);
        Log.println("WI-FI config configuration cleared.");
        isConfigCached = false;
        cachedConfig = Config{};
    } else {
        Log.println("No WI-FI config configuration to clear.");
    }
}

String WiFiConfigManager::trimString(const String& str) {
    String trimmed = str;
    trimmed.trim();
    return trimmed;
}
