// WiFiSetupManager.h
#ifndef WiFiSetupManager_h
#define WiFiSetupManager_h

#include "AbstractWiFiSetupManager.h"
#include "WiFiConfigManager.h"
#include "Config/RuntimeConfigManager.h"
#include <DNSServer.h>
#include <WebServer.h>

class WiFiSetupManager : public AbstractWiFiSetupManager {
public:
    WiFiSetupManager(WiFiConfigManager& configManager,
                     RuntimeConfigManager& runtimeConfig,
                     const char *apSsid, const char *apPassword);

    void initialize(SetupCompleteCallback callback) override;
    void handleClient() override;
    void stopServices() override;

private:
    const char *apSsid;
    const char *apPassword;
    WiFiConfigManager& configManager;
    RuntimeConfigManager& runtimeConfig;
    DNSServer dnsServer;
    WebServer webServer;
    SetupCompleteCallback setupCompleteCallback;
    void startAPMode();
    void setupCaptivePortal();
    void handleRoot();
    void handleScanNetworks();
    void handleConnectToNetwork();
    void handleNotFound();
    void handleGetAdvanced();
    void saveAdvancedFromForm();
};

#endif
