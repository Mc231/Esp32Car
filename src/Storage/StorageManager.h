// StorageManager.h
#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <string>

class StorageManager {
public:
    virtual ~StorageManager() = default;
    virtual std::string readFile(const std::string& fileName) = 0;
};

#endif // STORAGEMANAGER_H
