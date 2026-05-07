#ifndef ROVERWEBSOCKETSERVER_H
#define ROVERWEBSOCKETSERVER_H

#include <Arduino.h>
#include "Command/CommandDispatcher.h"
#include "Config/RoverApplicationConfig.h"
#include <WebSocketsServer.h>

class RoverWebSocketServer {
public:
  RoverWebSocketServer(CommandDispatcher& dispatcher, const RoverApplicationConfig& config);
  void begin();
  void loop();
  // Push an unsolicited message to all connected clients (used by other
  // transports that want to mirror state into the WS surface, if needed).
  void broadcast(const std::string& payload);
private:
  WebSocketsServer wsServer;
  CommandDispatcher& dispatcher;
  void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
};

#endif // ROVERWEBSOCKETSERVER_H
