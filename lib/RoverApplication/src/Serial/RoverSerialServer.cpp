#include "RoverSerialServer.h"
#include "Log/RemoteLogger.h"

RoverSerialServer::RoverSerialServer(CommandDispatcher& dispatcher)
  : dispatcher(dispatcher) {}

void RoverSerialServer::begin() {
  if (running) return;
  buf = "";
  running = true;
  Log.println("[serial] started — send JSON-per-line; replies prefixed with '>>> '");
}

void RoverSerialServer::stop() {
  if (!running) return;
  running = false;
  buf = "";
  Log.println("[serial] stopped");
}

void RoverSerialServer::loop() {
  if (!running) return;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (buf.length() > 0) {
        // Only treat lines that look like JSON as commands; ignore log
        // lines or stray input. Cheap discriminator that avoids needing
        // a separate UART for control.
        if (buf[0] == '{') {
          std::string raw(buf.c_str());
          dispatcher.dispatchRaw(raw, [](const std::string& reply) {
            Serial.print(">>> ");
            Serial.println(reply.c_str());
          });
        }
        buf = "";
      }
    } else {
      buf += c;
      if (buf.length() > MAX_LINE) {
        Log.println("[serial] input line too long, discarding");
        buf = "";
      }
    }
  }
}
