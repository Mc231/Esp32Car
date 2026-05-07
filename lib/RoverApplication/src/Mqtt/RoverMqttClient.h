#ifndef ROVERMQTTCLIENT_H
#define ROVERMQTTCLIENT_H

#ifdef ROVER_FEATURE_MQTT

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <string>
#include "Command/CommandDispatcher.h"
#include "Command/ICommandTransport.h"

// MQTT bridge for the rover's command surface.
//
// Topic layout (prefix is configurable, default "rover", id from MAC):
//   <prefix>/<id>/cmd        ← we subscribe; phones publish JSON commands
//   <prefix>/<id>/response   → we publish JSON replies + ad-hoc state
//   <prefix>/<id>/telemetry  → we publish periodic system+motor+distance
//   <prefix>/<id>/status     → "online" (retained); LWT publishes "offline"
class RoverMqttClient : public ICommandTransport {
public:
  struct Config {
    std::string host;
    uint16_t    port = 1883;
    std::string user;
    std::string password;
    std::string clientId;       // empty → derived from MAC
    std::string topicPrefix;    // empty → "rover"
    uint32_t    telemetryIntervalMs = 2000;
    uint32_t    reconnectIntervalMs = 5000;
  };

  RoverMqttClient(CommandDispatcher& dispatcher, const Config& cfg);

  // ICommandTransport
  const char* name() const override { return "mqtt"; }
  void begin() override;
  void stop() override;
  void loop() override;
  bool isRunning() const override { return enabled; }

private:
  bool enabled = false;
  CommandDispatcher& dispatcher;
  Config cfg;
  WiFiClient    netClient;
  PubSubClient  mqtt;

  std::string clientId;
  std::string topicCmd;
  std::string topicResponse;
  std::string topicTelemetry;
  std::string topicStatus;

  unsigned long lastReconnectAttempt = 0;
  unsigned long lastTelemetryAt = 0;

  void buildTopics();
  bool connect();
  static void onMessageThunk(char* topic, byte* payload, unsigned int length);
  void onMessage(const char* topic, byte* payload, unsigned int length);
  void publishTelemetry();
};

#endif // ROVER_FEATURE_MQTT
#endif // ROVERMQTTCLIENT_H
