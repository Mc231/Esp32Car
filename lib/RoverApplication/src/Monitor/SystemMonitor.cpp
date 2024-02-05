#include "SystemMonitor.h"

SystemMonitor::SystemMonitor() { }

int SystemMonitor::getUptimeInSeconds() const {
    return static_cast<int>(esp_timer_get_time() / 1000000ULL);
}

int SystemMonitor::getFreeHeapMemory() const {
    return static_cast<int>(esp_get_free_heap_size());
}

std::map<std::string, std::any> SystemMonitor::getState() const {
    std::map<std::string, std::any> state;
    state["up_time"] = getUptimeInSeconds();
    state["free_heap"] = getFreeHeapMemory();
    return state;
}