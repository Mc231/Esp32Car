#ifndef APPINITIALIZER_H
#define APPINITIALIZER_H

#include "Config/ConfigManager.h"
#include "CarApplication.h"
#include "Storage/SPIFFSStorageManager.h"

class AppInitializer {
public:
    AppInitializer(const std::string& configName); // Constructor now takes configName
    CarApplication* initializeApplication();

private:
    SPIFFSStorageManager storageManager;  
    ConfigManager configManager;
};

#endif // APPINITIALIZER_H
