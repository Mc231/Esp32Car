// MDNSBroadcaster.cpp
#include "MDNSBroadcaster.h"

MDNSBroadcaster::MDNSBroadcaster(const char *mdnsDiscoveryName, std::vector<MDNSService> services)
    : mdnsDiscoveryName(mdnsDiscoveryName), services(std::move(services)) { }

void MDNSBroadcaster::begin() {
    if (!MDNS.begin(this->mdnsDiscoveryName)) {
        Serial.println("Error setting up MDNS responder!");
        return;
    }
    Serial.println("mDNS responder started");
    for (const auto& s : services) {
        MDNS.addService(s.serviceType, s.protocol, s.port);
        Serial.printf("  advertising _%s._%s on %u\n", s.serviceType, s.protocol, s.port);
    }
}
