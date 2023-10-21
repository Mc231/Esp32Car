#include "AppInitializer.h"

AppInitializer::AppInitializer(const std::string& configName)
    : storageManager(),   
      configManager(storageManager, configName) 
{} 

CarApplication* AppInitializer::initializeApplication() {
    Serial.begin(115200);
    if (!configManager.loadConfig()) {
        Serial.println("Error: Failed to load configuration.");
        return nullptr;
    }
    Serial.println("Start loading config");
    CarApplicationConfig carAppConfig = configManager.getCarApplicationConfig();
    Serial.println("Config Loaded"); 
    return new CarApplication(carAppConfig, configManager);
}
