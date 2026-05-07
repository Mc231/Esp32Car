#ifndef ROVERWEBSERVER_H
#define ROVERWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config/RoverApplicationConfig.h"
#include "Command/CommandDispatcher.h"

// HTTP transport — exposes the unified JSON command surface plus the
// firmware utility pages (OTA upload, log viewer). The legacy per-route
// REST handlers (/motor, /system, …) and the firmware-served control
// HTML page were removed; the standalone `web/` app is the UI now and
// new clients should POST `/api/cmd`.
class RoverWebServer {
public:
  RoverWebServer(const RoverApplicationConfig& config, CommandDispatcher& dispatcher);
  void begin();
  void handleClient();

private:
  RoverApplicationConfig config;
  CommandDispatcher& dispatcher;
  WebServer server;

  void handleApiCommand();      // POST /api/cmd  — JSON in, JSON out
  void handleOtaPage();         // GET  /ota
  void handleOtaUpload();       // POST /ota/upload (multipart)
  void handleOtaUploadFinish();
  void handleLogsPage();        // GET  /logs
  void handleLogsData();        // GET  /logs/data?since=N

  bool authorized();            // 401 + return false when admin auth fails
};

#endif // ROVERWEBSERVER_H
