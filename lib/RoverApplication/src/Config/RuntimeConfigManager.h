#ifndef RUNTIMECONFIGMANAGER_H
#define RUNTIMECONFIGMANAGER_H

#include <Arduino.h>
#include "Fs/AbstractFS.h"

// Persists runtime-configurable settings (MQTT broker, BLE PIN) to SPIFFS.
// Provisioned via the captive portal "Advanced" section.
//
// Format: a single JSON file at `/runtime_config.cfg`. All fields are
// optional — if missing, the in-memory defaults stand and features stay
// disabled.
class RuntimeConfigManager {
public:
  struct Config {
    bool   mqttEnabled = false;
    String mqttHost;
    int    mqttPort = 1883;
    String mqttUser;
    String mqttPassword;
    String mqttClientId;       // empty → derived from MAC at runtime
    String mqttTopicPrefix;    // empty → "rover"

    bool   bleEnabled = false;
    String blePin = "123456";  // 6-digit static passkey
    String bleDeviceName;      // empty → derived from mdnsDiscoveryName
  };

  explicit RuntimeConfigManager(AbstractFS& fs);

  const Config& read();
  void save(const Config& cfg);
  void clear();

private:
  static constexpr const char* CONFIG_FILE = "/runtime_config.cfg";
  AbstractFS& fileSystem;
  Config cached;
  bool cachedLoaded = false;
  bool fsMounted = false;
  void ensureMounted();
};

#endif // RUNTIMECONFIGMANAGER_H
