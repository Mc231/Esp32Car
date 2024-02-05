// AbstractWiFiSetupManager.h
#ifndef AbstractWiFiSetupManager_h
#define AbstractWiFiSetupManager_h

#include <functional>

class AbstractWiFiSetupManager {
public:
    typedef std::function<void()> SetupCompleteCallback;

    virtual void initialize(SetupCompleteCallback callback) = 0;
    virtual void handleClient() = 0;
    virtual void stopServices() = 0;
    virtual ~AbstractWiFiSetupManager() {}
};

#endif
