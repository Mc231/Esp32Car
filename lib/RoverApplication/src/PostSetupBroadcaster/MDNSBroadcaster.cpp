// MDNSBroadcaster.cpp
#include "MDNSBroadcaster.h"

MDNSBroadcaster::MDNSBroadcaster(const char *mdnsDiscoveryName): mdnsDiscoveryName(mdnsDiscoveryName) { }

void MDNSBroadcaster::begin() {
    if (!MDNS.begin(this->mdnsDiscoveryName)) {
        Serial.println("Error setting up MDNS responder!");
        return;
    } 
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
}
