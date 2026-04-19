#ifndef REMOTE_LOGGER_H
#define REMOTE_LOGGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

class RemoteLogger;
// Global instance defined in RoverApplication.cpp.
extern RemoteLogger Log;

// A small Print subclass that mirrors writes to Serial AND any TCP clients
// connected to a telnet-style port (default 23). Keeps a small ring buffer
// so newly connected clients see the most recent output too.
//
// Usage:
//   RemoteLogger Log(23);            // global
//   Log.begin();                     // call after Wi-Fi is up
//   Log.loop();                      // call every loop()
//   Log.println("hello world");      // goes to Serial AND any telnet clients
class RemoteLogger : public Print {
public:
  RemoteLogger(uint16_t port = 23, size_t bufferSize = 2048, uint8_t maxClients = 3);
  void begin();
  void loop();

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
  std::vector<WiFiClient> clients;

  // Simple ring buffer (no allocation in the hot path)
  uint8_t* ring = nullptr;
  size_t ringHead = 0;     // next write position
  bool ringFull = false;
  size_t totalBytes = 0;   // monotonic write counter for cursor-based reads

  void appendToRing(uint8_t c);
  void sendRingTo(WiFiClient& c);
  void broadcast(const uint8_t* data, size_t len);
};

#endif
