// MDNSBroadcaster.cpp
#include "MDNSBroadcaster.h"
#include "Log/RemoteLogger.h"

MDNSBroadcaster::MDNSBroadcaster(const char *mdnsDiscoveryName, std::vector<MDNSService> services)
    : mdnsDiscoveryName(mdnsDiscoveryName), services(std::move(services)) { }

void MDNSBroadcaster::begin() {
    if (!MDNS.begin(this->mdnsDiscoveryName)) {
        Log.println("Error setting up MDNS responder!");
        return;
    }
    Log.println("mDNS responder started");
    for (const auto& s : services) {
        MDNS.addService(s.serviceType, s.protocol, s.port);
        Log.printf("  advertising _%s._%s on %u\n", s.serviceType, s.protocol, s.port);
    }
}
