#ifndef TRANSPORTREGISTRY_H
#define TRANSPORTREGISTRY_H

#include <Arduino.h>
#include <vector>
#include "ICommandTransport.h"

// Owns the set of ICommandTransport instances and exposes uniform
// lifecycle + name-based lookup. Keeps RoverApplication free of
// per-transport ifdef ladders — adding a new transport is one
// `add(...)` call.
//
// Ownership note: the registry holds non-owning pointers. RoverApplication
// owns the actual transport instances (heap-allocated for the optional
// ones, member instances for the always-on ones).
class TransportRegistry {
public:
  void add(ICommandTransport* t);
  void beginAll();
  void loopAll();
  void stopAll();

  ICommandTransport* find(const char* name);

  size_t size() const { return transports.size(); }
  ICommandTransport* at(size_t i) { return transports[i]; }

private:
  std::vector<ICommandTransport*> transports;
};

#endif // TRANSPORTREGISTRY_H
