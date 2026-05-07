#include "RoverWebSocketServer.h"
#include "Log/RemoteLogger.h"

RoverWebSocketServer::RoverWebSocketServer(CommandDispatcher& dispatcher, const RoverApplicationConfig& config)
  : wsServer(config.webSocketPort), dispatcher(dispatcher) {}

void RoverWebSocketServer::begin() {
  wsServer.begin();
  wsServer.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    this->handleWebSocketMessage(num, type, payload, length);
  });
}

void RoverWebSocketServer::loop() {
  wsServer.loop();
}

void RoverWebSocketServer::broadcast(const std::string& payload) {
  wsServer.broadcastTXT(payload.c_str(), payload.size());
}

void RoverWebSocketServer::handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
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
