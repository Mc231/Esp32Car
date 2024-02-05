// AbstractFS.h
#ifndef AbstractFS_h
#define AbstractFS_h

#include <Arduino.h>
#include <FS.h>

class AbstractFS {
public:
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual bool exists(const String& path) = 0;
    virtual File open(const String& path, const String& mode) = 0;
    virtual bool remove(const String& path) = 0;
};

#endif
