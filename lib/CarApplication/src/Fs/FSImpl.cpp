// LittleFSImpl.cpp
#include "FSImpl.h"
#include <FS.h>  

bool FSImpl::begin() {
    return SPIFFS.begin();
}

void FSImpl::end() {
    SPIFFS.end();
}

bool FSImpl::exists(const String& path) {
    return SPIFFS.exists(path);
}

File FSImpl::open(const String& path, const String& mode) {
    return SPIFFS.open(path.c_str(), mode.c_str());
}

bool FSImpl::remove(const String& path) {
    return SPIFFS.remove(path);
}