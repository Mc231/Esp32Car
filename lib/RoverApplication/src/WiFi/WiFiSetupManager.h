// WiFiSetupManager.h
#ifndef WiFiSetupManager_h
#define WiFiSetupManager_h

#include "AbstractWiFiSetupManager.h"
#include "WiFiConfigManager.h"
#include <DNSServer.h>
#include <WebServer.h>

class WiFiSetupManager : public AbstractWiFiSetupManager {
public:
    WiFiSetupManager(WiFiConfigManager& configManager, const char *apSsid, const char *apPassword);

    void initialize(SetupCompleteCallback callback) override;
    void handleClient() override;
    void stopServices() override;

private:
    const char *apSsid;
    const char *apPassword;
    WiFiConfigManager& configManager;
    DNSServer dnsServer;
    WebServer webServer;
    SetupCompleteCallback setupCompleteCallback;
    void startAPMode();
    void setupCaptivePortal();
    void handleRoot();
    void handleScanNetworks();
    void handleConnectToNetwork();
    void handleNotFound();
};

#endif
