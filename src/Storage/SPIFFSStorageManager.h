// SPIFFSStorageManager.h
#ifndef SPIFFSSTORAGEMANAGER_H
#define SPIFFSSTORAGEMANAGER_H

#include "StorageManager.h"
#include <SPIFFS.h>

class SPIFFSStorageManager : public StorageManager {
public:
    SPIFFSStorageManager();
    std::string readFile(const std::string& fileName) override;
};

#endif // SPIFFSSTORAGEMANAGER_H
