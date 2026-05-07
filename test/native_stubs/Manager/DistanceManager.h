// Native stub for DistanceManager. Tests poke the real RoverController
// stub which exposes getDistanceState() directly — the manager is only
// needed because RoverController's header forward-declares it.
#ifndef NATIVE_DISTANCEMANAGER_STUB_H
#define NATIVE_DISTANCEMANAGER_STUB_H

class DistanceManager {
public:
  void initialize() {}
  void update() {}
};

#endif
