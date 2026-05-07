#ifndef ROVERWEBSERVER_H
#define ROVERWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config/RoverApplicationConfig.h"
#include "Command/CommandDispatcher.h"
#include "Command/ICommandTransport.h"

// HTTP transport. Hosts:
//   - the unified `POST /api/cmd` JSON command surface (same shape as
//     WS / MQTT, dispatched through the shared CommandDispatcher)
//   - utility pages `/ota` (firmware upload) and `/logs` (live viewer)
//
// The standalone `web/` app is the canonical UI. There is no firmware
// -served control page or per-route REST API — clients send JSON to
// `/api/cmd` instead.
class RoverWebServer : public ICommandTransport {
public:
  RoverWebServer(const RoverApplicationConfig& config, CommandDispatcher& dispatcher);

  // ICommandTransport
  const char* name() const override { return "http"; }
  void begin() override;
  void stop() override;
  void loop() override { handleClient(); }
  bool isRunning() const override { return running; }

  void handleClient();

private:
  RoverApplicationConfig config;
  CommandDispatcher& dispatcher;
  WebServer server;
  bool running = false;

  void handleApiCommand();      // POST /api/cmd
  void handleOtaPage();
  void handleOtaUpload();
  void handleOtaUploadFinish();
  void handleLogsPage();
  void handleLogsData();

  bool authorized();            // 401 + return false on failed admin auth
};

#endif // ROVERWEBSERVER_H
