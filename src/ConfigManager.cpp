#include "ConfigManager.h"

ConfigManager::ConfigManager(StorageManager& storageManager)
: storageManager(storageManager) {}

bool ConfigManager::loadConfig(const std::string& filePath) {
    std::string content = storageManager.readFile(filePath);
    if(content.empty()) return false;

    // Split by lines and then by '=' to populate the map
    size_t pos = 0;
    while((pos = content.find('\n')) != std::string::npos) {
        std::string line = content.substr(0, pos);
        size_t equalPos = line.find('=');
        if(equalPos != std::string::npos) {
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);
            configData[key] = value;
        }
        content.erase(0, pos + 1);
    }
    return true;
}

std::map<std::string, std::string> ConfigManager::getConfig() {
    return configData;
}

std::string ConfigManager::getConfigValue(const std::string& key) {
    if(configData.find(key) != configData.end()) {
        return configData[key];
    }
    return "";
}
