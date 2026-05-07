#ifndef REMOTE_LOGGER_H
#define REMOTE_LOGGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "Command/ICommandTransport.h"

class CommandDispatcher;   // forward — full def in the .cpp
class RemoteLogger;
// Global instance defined in RoverApplication.cpp.
extern RemoteLogger Log;

// Bidirectional telnet (TCP) server on a configurable port (default 23).
//
// Mirrors writes to Serial AND any connected TCP clients (it's a Print
// subclass). Clients can also send JSON commands back: lines starting
// with `{` are parsed as JSON, dispatched through CommandDispatcher
// (when one has been wired via setDispatcher), and replies are
// broadcast back to all connected clients with a `>>> ` prefix —
// matching the Serial transport's wire format.
//
// Implements ICommandTransport (name="telnet") so the registry can
// lifecycle and toggle it like any other transport.
//
// Usage:
//   RemoteLogger Log(23);            // global
//   Log.begin();                     // call after Wi-Fi is up
//   Log.setDispatcher(&dispatcher);  // optional — enables command path
//   Log.loop();                      // call every loop()
//   Log.println("hello world");      // goes to Serial AND telnet clients
class RemoteLogger : public Print, public ICommandTransport {
public:
  RemoteLogger(uint16_t port = 23, size_t bufferSize = 2048, uint8_t maxClients = 3);
  ~RemoteLogger() override = default;

  // Wire a dispatcher to enable the read+command path. Without one,
  // RemoteLogger remains a one-way log push.
  void setDispatcher(CommandDispatcher* d) { dispatcher = d; }

  // ICommandTransport
  const char* name() const override { return "telnet"; }
  void begin() override;
  void stop() override;
  void loop() override;
  bool isRunning() const override { return started; }

  // Print interface
  size_t write(uint8_t c) override;
  size_t write(const uint8_t* buffer, size_t size) override;

  // For HTTP /logs endpoint: total bytes ever written + read since cursor.
  // Cursor is a monotonic byte offset; pass 0 to get the whole ring.
  size_t totalWritten() const { return totalBytes; }
  String readSince(size_t cursor) const;

private:
  uint16_t port;
  size_t bufferSize;
  uint8_t maxClients;
  bool started = false;

  WiFiServer server;
  CommandDispatcher* dispatcher = nullptr;

  struct Client {
    WiFiClient sock;
    String inputBuf;
  };
  std::vector<Client> clients;

  // Simple ring buffer (no allocation in the hot path)
  uint8_t* ring = nullptr;
  size_t ringHead = 0;     // next write position
  bool ringFull = false;
  size_t totalBytes = 0;   // monotonic write counter for cursor-based reads

  void appendToRing(uint8_t c);
  void sendRingTo(WiFiClient& c);
  void broadcast(const uint8_t* data, size_t len);
  void handleClientInput(Client& c);
};

#endif
