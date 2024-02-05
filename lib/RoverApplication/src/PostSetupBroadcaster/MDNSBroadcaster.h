// MDNSBroadcaster.h
#ifndef MDNSBroadcaster_h
#define MDNSBroadcaster_h
#include <ESPmDNS.h>

#include "PostSetupAvailabilityBroadcaster.h"

class MDNSBroadcaster : public PostSetupAvailabilityBroadcaster {
public:
    MDNSBroadcaster(const char *mdnsDiscoveryName);
    void begin() override;

private:
    const char *mdnsDiscoveryName;

};

#endif
   