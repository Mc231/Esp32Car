#include "ConfigManager.h"

ConfigManager::ConfigManager(StorageManager& storageManager, const std::string& filePath)
    : storageManager(storageManager), filePath(filePath) {}

bool ConfigManager::loadConfig() {
    if (configData.empty()) {
        std::string content = storageManager.readFile(filePath);
        if (content.empty()) {
            return false;
        }

        // Split by lines and then by '=' to populate the map
        size_t pos = 0;
        while ((pos = content.find('\n')) != std::string::npos) {
            std::string line = content.substr(0, pos);
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);
                configData[key] = value;
            }
            content.erase(0, pos + 1);
        }

        if (!content.empty()) {
            size_t equalPos = content.find('=');
            if (equalPos != std::string::npos) {
                std::string key = content.substr(0, equalPos);
                std::string value = content.substr(equalPos + 1);
                configData[key] = value;
            }
        }
    }

    return true;
}

std::map<std::string, std::string> ConfigManager::getConfig() {
    return configData;
}

std::string ConfigManager::getConfigValue(const std::string& key) {
    if (configData.find(key) != configData.end()) {
        return configData[key];
    }
    return "";
}

CarApplicationConfig ConfigManager::getCarApplicationConfig() {
    if (configData.empty()) {
        loadConfig();
    }

    if (config.ssid.empty()) {  // You can check any member variable of config to determine if it's loaded
        config.ssid = getConfigValue("SSID");
        config.ssidPassword = getConfigValue("SSID_PASSWORD");
        config.leftMotorPin1 = std::stoi(getConfigValue("LEFT_MOTOR_PIN_1"));
        config.leftMotorPin2 = std::stoi(getConfigValue("LEFT_MOTOR_PIN_2"));
        config.leftMotorPwm = std::stoi(getConfigValue("LEFT_MOTOR_PWM"));
        config.rightMotorPin1 = std::stoi(getConfigValue("RIGHT_MOTOR_PIN_1"));
        config.rightMotorPin2 = std::stoi(getConfigValue("RIGHT_MOTOR_PIN_2"));
        config.rightMotorPwm = std::stoi(getConfigValue("RIGHT_MOTOR_PWM"));
        config.ultrasonicPin1 = std::stoi(getConfigValue("ULTRASONIC_PIN_1"));
        config.ultrasonicPin2 = std::stoi(getConfigValue("ULTRASONIC_PIN_2"));
    }

    return config;
}
