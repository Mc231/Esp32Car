#include "AppInitializer.h"

AppInitializer::AppInitializer()
    : storageManager(),   
      configManager(storageManager) 
{} 

CarApplication* AppInitializer::initializeApplication(const std::string& configName) {
    if (!configManager.loadConfig(configName)) {
        Serial.println("Error: Failed to load configuration.");
        return nullptr;
    }
      std::map<std::string, std::string> configEntries = configManager.getConfig();
    for (const auto& entry : configEntries) {
        Serial.print("Key: ");
        Serial.print(entry.first.c_str());
        Serial.print(", Value: ");
        Serial.println(entry.second.c_str());
    }
        Serial.print("Free heap memory: ");
    Serial.println(ESP.getFreeHeap());
    CarApplicationConfig carAppConfig;
    carAppConfig.ssid = configManager.getConfigValue("SSID").c_str();
    carAppConfig.ssidPassword = configManager.getConfigValue("SSID_PASSWORD").c_str();
    carAppConfig.horizontalServoPin = std::stoi(configManager.getConfigValue("HORIZONTAL_SERVO_PIN"));
    carAppConfig.verticalServoPin = std::stoi(configManager.getConfigValue("VERTICAL_SERVO_PIN"));

    carAppConfig.leftMotorPin1 = std::stoi(configManager.getConfigValue("LEFT_MOTOR_PIN_1"));
    carAppConfig.leftMotorPin2 = std::stoi(configManager.getConfigValue("LEFT_MOTOR_PIN_2"));
    carAppConfig.leftMotorPwm = std::stoi(configManager.getConfigValue("LEFT_MOTOR_PWM"));
    carAppConfig.rightMotorPin1 = std::stoi(configManager.getConfigValue("RIGHT_MOTOR_PIN_1"));
    carAppConfig.rightMotorPin2 = std::stoi(configManager.getConfigValue("RIGHT_MOTOR_PIN_2"));
    carAppConfig.rightMotorPwm = std::stoi(configManager.getConfigValue("RIGHT_MOTOR_PWM"));
    carAppConfig.ultrasonicPin1 = std::stoi(configManager.getConfigValue("ULTRASONIC_PIN_1"));
    carAppConfig.ultrasonicPin2 = std::stoi(configManager.getConfigValue("ULTRASONIC_PIN_2"));
    Serial.println("Config Loaded"); 
    return new CarApplication(carAppConfig);
}


