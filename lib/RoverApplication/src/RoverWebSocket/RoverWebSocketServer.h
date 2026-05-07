#ifndef ROVERWEBSOCKETSERVER_H
#define ROVERWEBSOCKETSERVER_H

#include <Arduino.h>
#include "Command/CommandDispatcher.h"
#include "Command/ICommandTransport.h"
#include "Config/RoverApplicationConfig.h"
#include <WebSocketsServer.h>

class RoverWebSocketServer : public ICommandTransport {
public:
  RoverWebSocketServer(CommandDispatcher& dispatcher, const RoverApplicationConfig& config);

  // ICommandTransport
  const char* name() const override { return "ws"; }
  void begin() override;
  void stop() override;
  void loop() override;
  bool isRunning() const override { return running; }

  // Push an unsolicited message to all connected clients.
  void broadcast(const std::string& payload);

private:
  WebSocketsServer wsServer;
  CommandDispatcher& dispatcher;
  bool running = false;
  void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
};

#endif // ROVERWEBSOCKETSERVER_H
