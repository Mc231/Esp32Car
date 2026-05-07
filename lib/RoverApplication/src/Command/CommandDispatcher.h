#ifndef COMMANDDISPATCHER_H
#define COMMANDDISPATCHER_H

#include <Arduino.h>
#include <functional>
#include <map>
#include <any>
#include <string>
#include <nlohmann/json.hpp>
#include "Controller/RoverController.h"
#include "Config/RoverApplicationConfig.h"
#include "Monitor/SystemMonitor.h"
#include "TransportRegistry.h"

// Transport-agnostic JSON command dispatcher.
//
// The HTTP / WebSocket / MQTT transports are thin adapters — they receive
// raw bytes, hand them here, and pipe the response JSON back via the
// `respond` callback. Same command surface, three different pipes.
//
// Usage:
//   dispatcher.dispatchRaw("{\"command\":\"system\"}",
//                          [](const std::string& reply) { ws.send(reply); });
class CommandDispatcher {
public:
  using ReplyFn = std::function<void(const std::string&)>;

  CommandDispatcher(RoverController& controller,
                    const RoverApplicationConfig& config,
                    SystemMonitor& systemMonitor);

  // Parse `raw` as JSON, dispatch by `command` field, call `respond` zero or
  // more times with the JSON-serialized reply(s).
  void dispatchRaw(const std::string& raw, const ReplyFn& respond);

  // Build the telemetry payload (system + motor + distance) without going
  // through the command parsing path. Used by MQTT's periodic publisher.
  std::string buildTelemetry();

  // Optional registry wiring for the runtime transport-toggle commands.
  // When unset, `transports` and `set_transport` return an "unavailable"
  // status.
  void setTransportRegistry(TransportRegistry* r) { registry = r; }

private:
  RoverController& controller;
  RoverApplicationConfig config;
  SystemMonitor& systemMonitor;
  TransportRegistry* registry = nullptr;

  void handleSystem(const ReplyFn& respond);
  void handleStatus(const ReplyFn& respond);
  void handleConfig(const ReplyFn& respond);
  void handleReboot(const ReplyFn& respond);
  void handleGetWiFi(const ReplyFn& respond);
  void handleWiFiForget(const ReplyFn& respond);
  void handleGetDistance(const ReplyFn& respond);
  void handleCamera(const nlohmann::json& j, const ReplyFn& respond);
  void handleSetMotorPWM(const nlohmann::json& j, const ReplyFn& respond);
  void handleMotorState(const ReplyFn& respond);
  void handleSetMotor(const nlohmann::json& j, const ReplyFn& respond);
  void handleListTransports(const ReplyFn& respond);
  void handleSetTransport(const nlohmann::json& j, const ReplyFn& respond);

  // Build a JSON-serialized response from a std::map<std::string, std::any>.
  std::string serialize(const std::map<std::string, std::any>& dataMap) const;
  nlohmann::json asJSON(const std::map<std::string, std::any>& map) const;
};

#endif // COMMANDDISPATCHER_H
