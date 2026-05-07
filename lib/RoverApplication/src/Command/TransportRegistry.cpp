#include "TransportRegistry.h"
#include <cstring>
#include "Log/RemoteLogger.h"

void TransportRegistry::add(ICommandTransport* t) {
  if (!t) return;
  transports.push_back(t);
}

void TransportRegistry::beginAll() {
  for (auto* t : transports) {
    Log.printf("[transport] begin: %s\n", t->name());
    t->begin();
  }
}

void TransportRegistry::loopAll() {
  for (auto* t : transports) t->loop();
}

void TransportRegistry::stopAll() {
  for (auto* t : transports) {
    if (t->isRunning()) {
      Log.printf("[transport] stop: %s\n", t->name());
      t->stop();
    }
  }
}

ICommandTransport* TransportRegistry::find(const char* needle) {
  if (!needle) return nullptr;
  for (auto* t : transports) {
    if (std::strcmp(t->name(), needle) == 0) return t;
  }
  return nullptr;
}
