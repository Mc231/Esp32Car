// WiFiConfigManager.h
#ifndef WiFiConfigManager_h
#define WiFiConfigManager_h
#include <FS.h>
#include "Fs/AbstractFS.h"

class WiFiConfigManager {
public:
    struct Config {
        String ssid;
        String password;
    };

    WiFiConfigManager(AbstractFS& fs);

    Config readConfig();
    void clearConfig();
    void saveConfig(const String& ssid, const String& password);

private:
    static constexpr const char* WIFI_CONFIG_FILE = "/wi_fi_config.cfg";
    AbstractFS& fileSystem;
    bool isConfigCached;
    Config cachedConfig;
    String trimString(const String& str);
};

#endif
