#include "SDCardStorageManager.h"
#include "FS.h"
#include "SD_MMC.h"

SDCardStorageManager::SDCardStorageManager() {
    if (!SD.begin()) {
        Serial.println("Failed to initialize SD card.");
    }
     uint8_t cardType = SD_MMC.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD_MMC card attached");
    return;
  }
}

std::string SDCardStorageManager::readFile(const std::string& fileName) {
    File file = SD.open(fileName.c_str());
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
