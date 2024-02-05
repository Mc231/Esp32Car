#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <esp_timer.h>
#include <esp_system.h>
#include <map>
#include <any>

class SystemMonitor {
public:
    SystemMonitor();
    int getUptimeInSeconds() const;
    int getFreeHeapMemory() const;
    std::map<std::string, std::any> getState() const;
};

#endif // SYSTEMMONITOR_H
