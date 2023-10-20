#ifndef SDCARDSTORAGEMANAGER_H
#define SDCARDSTORAGEMANAGER_H

#include "StorageManager.h"
#include <SD.h>
#include <FS.h>

class SDCardStorageManager : public StorageManager {
public:
    SDCardStorageManager();
    std::string readFile(const std::string& fileName) override;
};

#endif // SDCARDSTORAGEMANAGER_H
