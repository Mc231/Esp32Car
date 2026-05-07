#ifndef ROVERSERIALSERVER_H
#define ROVERSERIALSERVER_H

#include <Arduino.h>
#include "Command/CommandDispatcher.h"
#include "Command/ICommandTransport.h"

// Serial (USB UART) transport for the unified JSON command surface.
//
// Wire format: one JSON object per line, terminated by '\n' (or '\r').
// Only lines that start with `{` are treated as commands — keeps logger
// output (which prints free-form lines) from being mis-parsed when both
// share the same UART.
//
// Replies are emitted with a `>>> ` prefix on their own line so a
// host-side parser can grep them out of interleaved log output:
//   >>> {"response":{...}}
class RoverSerialServer : public ICommandTransport {
public:
  explicit RoverSerialServer(CommandDispatcher& dispatcher);

  // ICommandTransport
  const char* name() const override { return "serial"; }
  void begin() override;
  void stop() override;
  void loop() override;
  bool isRunning() const override { return running; }

private:
  CommandDispatcher& dispatcher;
  bool running = false;
  String buf;             // accumulates the current line until '\n'
  static constexpr size_t MAX_LINE = 1024;
};

#endif // ROVERSERIALSERVER_H
