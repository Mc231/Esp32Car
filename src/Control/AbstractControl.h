#ifndef ABSTRACTCONTROL_H
#define ABSTRACTCONTROL_H

#include <ArduinoJson.h>
#include <map>
#include <any>
#include <string>

class AbstractControl {
public:
    virtual ~AbstractControl() = default;  // Virtual destructor to ensure proper cleanup for derived classes
    virtual std::map<std::string, std::any> getState() const = 0;  // Pure virtual method
};

#endif // ABSTRACTCONTROL_H
