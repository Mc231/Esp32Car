// MDNSBroadcaster.h
#ifndef MDNSBroadcaster_h
#define MDNSBroadcaster_h
#include <ESPmDNS.h>
#include <vector>

#include "PostSetupAvailabilityBroadcaster.h"

struct MDNSService {
    const char* serviceType;
    const char* protocol;
    uint16_t port;
};

class MDNSBroadcaster : public PostSetupAvailabilityBroadcaster {
public:
    MDNSBroadcaster(const char *mdnsDiscoveryName,
                    std::vector<MDNSService> services = {{"http", "tcp", 80}});
    void begin() override;

private:
    const char *mdnsDiscoveryName;
    std::vector<MDNSService> services;
};

#endif
