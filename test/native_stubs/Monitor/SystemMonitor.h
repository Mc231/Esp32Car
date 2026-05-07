// Native stub of SystemMonitor with a programmable canned getState()
// return. Tests set `state` to whatever they want the dispatcher to
// serialize into the `system` / telemetry replies.
#ifndef NATIVE_SYSTEMMONITOR_STUB_H
#define NATIVE_SYSTEMMONITOR_STUB_H

#include <map>
#include <any>
#include <string>

class SystemMonitor {
public:
  std::map<std::string, std::any> state;
  std::map<std::string, std::any> getState() const { return state; }
};

#endif
