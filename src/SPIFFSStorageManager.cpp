// SPIFFSStorageManager.cpp
#include "SPIFFSStorageManager.h"

SPIFFSStorageManager::SPIFFSStorageManager() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
    }
}

std::string SPIFFSStorageManager::readFile(const std::string& fileName) {
    File file = SPIFFS.open(fileName.c_str());
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }
    std::string fileContent;
    while (file.available()) {
        fileContent += char(file.read());
    }
    file.close();
    return fileContent;
}
