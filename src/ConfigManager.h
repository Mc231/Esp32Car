#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "StorageManager.h"
#include <string>
#include <map>

class ConfigManager {
public:
    ConfigManager(StorageManager& storageManager);
    bool loadConfig(const std::string& filePath);
    std::string getConfigValue(const std::string& key);
    std::map<std::string, std::string> getConfig();

private:
    StorageManager& storageManager;
    std::map<std::string, std::string> configData;
};

#endif // CONFIGMANAGER_H