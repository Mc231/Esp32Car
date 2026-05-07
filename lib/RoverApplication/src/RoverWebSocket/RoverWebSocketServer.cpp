#include "RoverWebSocketServer.h"
#include "Log/RemoteLogger.h"

RoverWebSocketServer::RoverWebSocketServer(CommandDispatcher& dispatcher, const RoverApplicationConfig& config)
  : wsServer(config.webSocketPort), dispatcher(dispatcher) {}

void RoverWebSocketServer::begin() {
  if (running) return;
  wsServer.begin();
  wsServer.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    this->handleWebSocketMessage(num, type, payload, length);
  });
  running = true;
  Log.println("[ws] started");
}

void RoverWebSocketServer::stop() {
  if (!running) return;
  // Drop all clients. The listener stays bound to the port — Links2004's
  // WebSocketsServer doesn't expose a clean teardown — but `running`
  // gates message handling so new arrivals get nothing back. Reasonable
  // for a runtime toggle; full shutdown would require a heap-allocated
  // server we new/delete here.
  wsServer.disconnect();
  running = false;
  Log.println("[ws] stopped");
}

void RoverWebSocketServer::loop() {
  if (running) wsServer.loop();
}

void RoverWebSocketServer::broadcast(const std::string& payload) {
  if (running) wsServer.broadcastTXT(payload.c_str(), payload.size());
}

void RoverWebSocketServer::handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (!running) return;
  if (type != WStype_TEXT) return;
  payload[length] = 0;   // null-terminate
  std::string raw(reinterpret_cast<char*>(payload), length);
  // Existing protocol: replies are broadcast to all connected clients
  // (no per-request correlation id). Preserve that here so existing
  // clients keep working unchanged.
  dispatcher.dispatchRaw(raw, [this](const std::string& reply) {
    wsServer.broadcastTXT(reply.c_str(), reply.size());
  });
}
