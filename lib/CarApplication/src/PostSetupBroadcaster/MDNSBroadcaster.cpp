// MDNSBroadcaster.cpp
#include "MDNSBroadcaster.h"

void MDNSBroadcaster::begin() {
    if (!MDNS.begin("rover")) {
        Serial.println("Error setting up MDNS responder!");
        return;
    } 
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
}
