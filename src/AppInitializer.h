#ifndef APPINITIALIZER_H
#define APPINITIALIZER_H

#include "ConfigManager.h"
#include "CarApplication.h"
#include "SPIFFSStorageManager.h"

class AppInitializer {
public:
    AppInitializer();
    CarApplication* initializeApplication(const std::string& configName);

private:
    SPIFFSStorageManager storageManager;  
    ConfigManager configManager;
};

#endif // APPINITIALIZER_H