#ifdef ROVER_FEATURE_MQTT

#include "RoverMqttClient.h"
#include <WiFi.h>
#include "Log/RemoteLogger.h"

namespace {
// PubSubClient hands the receive callback through a C function pointer with
// no user-data slot. We park a single instance pointer here so the static
// thunk can route incoming messages to the right object. There's only ever
// one MQTT client per rover, so a singleton pointer is fine.
RoverMqttClient* g_instance = nullptr;
} // namespace

RoverMqttClient::RoverMqttClient(CommandDispatcher& dispatcher, const Config& cfg)
  : dispatcher(dispatcher), cfg(cfg), mqtt(netClient) {}

void RoverMqttClient::buildTopics() {
  // Derive id from MAC if not provided.
  if (cfg.clientId.empty()) {
    uint8_t mac[6]; WiFi.macAddress(mac);
    char buf[16];
    snprintf(buf, sizeof(buf), "rover-%02x%02x%02x", mac[3], mac[4], mac[5]);
    clientId = buf;
  } else {
    clientId = cfg.clientId;
  }
  std::string prefix = cfg.topicPrefix.empty() ? "rover" : cfg.topicPrefix;
  // The "id" segment in topics is the clientId (so it works whether
  // user picked a custom one or we derived from MAC).
  topicCmd       = prefix + "/" + clientId + "/cmd";
  topicResponse  = prefix + "/" + clientId + "/response";
  topicTelemetry = prefix + "/" + clientId + "/telemetry";
  topicStatus    = prefix + "/" + clientId + "/status";
}

void RoverMqttClient::begin() {
  if (enabled) return;
  if (cfg.host.empty()) {
    Log.println("[mqtt] no broker host configured — disabled");
    return;
  }
  g_instance = this;
  buildTopics();
  mqtt.setServer(cfg.host.c_str(), cfg.port);
  mqtt.setCallback(&RoverMqttClient::onMessageThunk);
  // PubSubClient default buffer is 256 B — too small for our combined
  // telemetry payload. Bump it.
  mqtt.setBufferSize(1024);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(5);
  enabled = true;
  Log.printf("[mqtt] started: %s:%u prefix=%s id=%s\n",
             cfg.host.c_str(), cfg.port,
             cfg.topicPrefix.empty() ? "rover" : cfg.topicPrefix.c_str(),
             clientId.c_str());
}

void RoverMqttClient::stop() {
  if (!enabled) return;
  if (mqtt.connected()) {
    // Best-effort: publish "offline" to status before tearing down so
    // subscribers see us go away cleanly (LWT also covers ungraceful drops).
    mqtt.publish(topicStatus.c_str(), "offline", true);
    mqtt.disconnect();
  }
  enabled = false;
  Log.println("[mqtt] stopped");
}

void RoverMqttClient::reconfigure(const Config& newCfg) {
  bool wasRunning = enabled;
  if (wasRunning) stop();
  cfg = newCfg;
  if (!cfg.host.empty()) {
    // Re-derive client id + topics from the (possibly new) prefix and id.
    // begin() also calls buildTopics(), but doing it here lets the new
    // values be observable even before begin() is called.
    buildTopics();
    mqtt.setServer(cfg.host.c_str(), cfg.port);
  }
  Log.printf("[mqtt] reconfigured: host=%s port=%u prefix=%s id=%s\n",
             cfg.host.c_str(), cfg.port,
             cfg.topicPrefix.empty() ? "rover" : cfg.topicPrefix.c_str(),
             clientId.c_str());
  // Caller decides whether to begin() again.
}

bool RoverMqttClient::connect() {
  if (cfg.host.empty()) return false;
  Log.printf("[mqtt] connecting to %s:%u as %s\n", cfg.host.c_str(), cfg.port, clientId.c_str());
  bool ok;
  if (cfg.user.empty()) {
    ok = mqtt.connect(clientId.c_str(),
                      /*lastWillTopic=*/topicStatus.c_str(),
                      /*lwQoS=*/0, /*lwRetain=*/true,
                      /*lwMessage=*/"offline");
  } else {
    ok = mqtt.connect(clientId.c_str(),
                      cfg.user.c_str(), cfg.password.c_str(),
                      topicStatus.c_str(),
                      0, true, "offline");
  }
  if (!ok) {
    Log.printf("[mqtt] connect failed, state=%d\n", mqtt.state());
    return false;
  }
  mqtt.publish(topicStatus.c_str(), "online", true);
  mqtt.subscribe(topicCmd.c_str());
  Log.printf("[mqtt] connected, subscribed to %s\n", topicCmd.c_str());
  return true;
}

void RoverMqttClient::loop() {
  if (!enabled) return;
  if (cfg.host.empty()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt < cfg.reconnectIntervalMs) return;
    lastReconnectAttempt = now;
    connect();
    return;
  }
  mqtt.loop();
  unsigned long now = millis();
  if (now - lastTelemetryAt >= cfg.telemetryIntervalMs) {
    lastTelemetryAt = now;
    publishTelemetry();
  }
}

void RoverMqttClient::publishTelemetry() {
  std::string payload = dispatcher.buildTelemetry();
  mqtt.publish(topicTelemetry.c_str(), payload.c_str());
}

void RoverMqttClient::onMessageThunk(char* topic, byte* payload, unsigned int length) {
  if (g_instance) g_instance->onMessage(topic, payload, length);
}

void RoverMqttClient::onMessage(const char* topic, byte* payload, unsigned int length) {
  if (topicCmd != topic) return;
  std::string raw(reinterpret_cast<const char*>(payload), length);
  dispatcher.dispatchRaw(raw, [this](const std::string& reply) {
    mqtt.publish(topicResponse.c_str(), reply.c_str());
  });
}

#endif // ROVER_FEATURE_MQTT
