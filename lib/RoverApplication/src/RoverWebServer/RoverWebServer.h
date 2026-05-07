#ifndef ROVERWEBSERVER_H
#define ROVERWEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "Controller/RoverController.h"
#ifndef ROVER_NO_CAMERA
#include "esp_camera.h"
#endif
#include "Config/RoverApplicationConfig.h"
#include "Monitor/SystemMonitor.h"
#include "Command/CommandDispatcher.h"
#include "Command/ICommandTransport.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// HTTP transport. Hosts:
//   - the firmware-served control HTML page at GET /
//   - the legacy per-route REST API the page uses (/motor, /motorPWM,
//     /system, /status, /distance, /wifi, /reboot, …) — kept so the
//     firmware UI works standalone, no external web app needed
//   - the unified `POST /api/cmd` JSON command surface that BLE/WS/MQTT
//     also expose, for clients that don't want one route per command
//   - utility pages `/ota` (firmware upload) and `/logs` (live viewer)
//
// Implements ICommandTransport so the registry can lifecycle it
// uniformly. HTTP is "always-on" by convention but the interface lets
// future callers stop/start it via the `set_transport` command.
class RoverWebServer : public ICommandTransport {
public:
  RoverWebServer(RoverController& carController,
                 const RoverApplicationConfig& config,
                 SystemMonitor& systemMonitor,
                 CommandDispatcher& dispatcher);

  // ICommandTransport
  const char* name() const override { return "http"; }
  void begin() override;
  void stop() override;
  void loop() override { handleClient(); }
  bool isRunning() const override { return running; }

  void handleClient();

  // Legacy REST handlers (used by the firmware-served HTML control page).
  void handleReboot();
  void handleGetWiFi();
  void handleWiFiForget();
  void handleSetMotor();
  void handleSetMotorPWM();
  void handleMotorState();
  void handleGetDistance();
  void handleCamera();
  void handleRoot();
  void handleSystem();
  void handleStatus();
  void handleConfig();

  // Unified JSON command surface (POST /api/cmd).
  void handleApiCommand();

  // Utility pages.
  void handleOtaPage();
  void handleOtaUpload();
  void handleOtaUploadFinish();
  void handleLogsPage();
  void handleLogsData();

private:
  RoverController& carController;
  RoverApplicationConfig config;
  SystemMonitor& systemMonitor;
  CommandDispatcher& dispatcher;
  WebServer server;
  bool running = false;

  json asJSON(const std::map<std::string, std::any>& map) const;
  void sendData(const std::map<std::string, std::any>& dataMap,
                const String& responseType = "application/json");
  bool authorized();   // 401 + return false on failed admin auth
};

#endif // ROVERWEBSERVER_H
