// LittleFSImpl.h
#ifndef FSImpl_h
#define FSImpl_h

#include "AbstractFS.h"
#include <SPIFFS.h>

class FSImpl : public AbstractFS {
public:
    bool begin() override;
    void end() override;
    bool exists(const String& path) override;
    File open(const String& path, const String& mode) override;
    bool remove(const String& path) override;
};

#endif
