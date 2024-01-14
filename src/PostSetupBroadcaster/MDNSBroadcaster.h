// MDNSBroadcaster.h
#ifndef MDNSBroadcaster_h
#define MDNSBroadcaster_h
#include <ESPmDNS.h>

#include "PostSetupAvailabilityBroadcaster.h"

class MDNSBroadcaster : public PostSetupAvailabilityBroadcaster {
public:
    void begin() override;
};

#endif
