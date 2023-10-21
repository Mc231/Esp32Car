#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Storage/StorageManager.h"
#include "CarApplicationConfig.h"
#include <string>
#include <map>
#include <any>

class ConfigManager {
public:
    ConfigManager(StorageManager& storageManager, const std::string& filePath);
    bool loadConfig();
    std::string getConfigValue(const std::string& key);
    std::map<std::string, std::any> getConfig();
    CarApplicationConfig getCarApplicationConfig();
private:
    StorageManager& storageManager;
    const std::string filePath;
    CarApplicationConfig config; 
    std::map<std::string, std::string> configData;
};

#endif // CONFIGMANAGER_H