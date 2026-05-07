#include "RemoteLogger.h"
#include "Command/CommandDispatcher.h"
#include <esp_log.h>
#include <stdarg.h>

RemoteLogger::RemoteLogger(uint16_t port, size_t bufferSize, uint8_t maxClients)
  : port(port), bufferSize(bufferSize), maxClients(maxClients), server(port) {}

// Bridge ESP-IDF's log output (ESP_LOGx + system errors) into our Logger.
static int espLogToRemote(const char* fmt, va_list args) {
  char buf[256];
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  if (n < 0) return n;
  if ((size_t)n >= sizeof(buf)) n = sizeof(buf) - 1;
  Log.write((const uint8_t*)buf, n);
  return n;
}

void RemoteLogger::begin() {
  if (started) return;
  if (!ring) {
    ring = (uint8_t*)malloc(bufferSize);
    if (!ring) {
      Serial.println("RemoteLogger: malloc failed, telnet log disabled");
      return;
    }
  }
  server.begin();
  server.setNoDelay(true);
  started = true;
  esp_log_set_vprintf(&espLogToRemote);
  Serial.printf("RemoteLogger: telnet on port %u (use: nc <ip> %u)\n", port, port);
}

void RemoteLogger::stop() {
  if (!started) return;
  for (auto& c : clients) {
    if (c.sock.connected()) c.sock.stop();
  }
  clients.clear();
  server.end();
  started = false;
}

void RemoteLogger::loop() {
  if (!started) return;

  // Accept new clients
  if (server.hasClient()) {
    WiFiClient newClient = server.accept();
    if (clients.size() >= maxClients) {
      newClient.println("Sorry, too many telnet clients. Disconnect another and retry.");
      newClient.stop();
    } else {
      newClient.setNoDelay(true);
      sendRingTo(newClient);
      newClient.println("--- live log ---");
      if (dispatcher) {
        newClient.println("(type a JSON command + Enter to control the rover; replies prefixed with '>>> ')");
      }
      clients.push_back(Client{ std::move(newClient), String() });
    }
  }

  // Drop disconnected clients + read pending input from the rest.
  for (auto it = clients.begin(); it != clients.end(); ) {
    if (!it->sock.connected()) {
      it->sock.stop();
      it = clients.erase(it);
    } else {
      handleClientInput(*it);
      ++it;
    }
  }
}

void RemoteLogger::handleClientInput(Client& c) {
  while (c.sock.available()) {
    char ch = (char)c.sock.read();
    if (ch == '\n' || ch == '\r') {
      if (c.inputBuf.length() > 0) {
        // Only treat lines that look like JSON as commands; ignore stray
        // input or the user pressing Enter on a blank line.
        if (c.inputBuf[0] == '{' && dispatcher) {
          std::string raw(c.inputBuf.c_str());
          dispatcher->dispatchRaw(raw, [this](const std::string& reply) {
            // Broadcast reply to all telnet clients, matching the WS
            // pattern (every connected client sees every reply). Prefix
            // mirrors the Serial transport so host-side parsers can
            // grep replies out of interleaved log output.
            std::string line = ">>> " + reply + "\r\n";
            broadcast((const uint8_t*)line.data(), line.size());
          });
        }
        c.inputBuf = "";
      }
    } else {
      c.inputBuf += ch;
      if (c.inputBuf.length() > 1024) {
        c.sock.println("Input line too long, discarded.");
        c.inputBuf = "";
      }
    }
  }
}

size_t RemoteLogger::write(uint8_t c) {
  Serial.write(c);
  appendToRing(c);
  if (started) broadcast(&c, 1);
  return 1;
}

size_t RemoteLogger::write(const uint8_t* buffer, size_t size) {
  Serial.write(buffer, size);
  for (size_t i = 0; i < size; ++i) appendToRing(buffer[i]);
  if (started) broadcast(buffer, size);
  return size;
}

void RemoteLogger::appendToRing(uint8_t c) {
  if (!ring) return;
  ring[ringHead++] = c;
  totalBytes++;
  if (ringHead >= bufferSize) {
    ringHead = 0;
    ringFull = true;
  }
}

String RemoteLogger::readSince(size_t cursor) const {
  if (!ring) return String();
  size_t available = totalBytes - cursor;
  if (available == 0) return String();
  if (available > bufferSize) available = bufferSize;   // we lost some — give what we have

  String out;
  out.reserve(available + 1);
  size_t start = (ringHead + bufferSize - available) % bufferSize;
  for (size_t i = 0; i < available; ++i) {
    out += (char)ring[(start + i) % bufferSize];
  }
  return out;
}

void RemoteLogger::sendRingTo(WiFiClient& client) {
  if (!ring) return;
  if (ringFull) {
    client.write(ring + ringHead, bufferSize - ringHead);
    client.write(ring, ringHead);
  } else if (ringHead > 0) {
    client.write(ring, ringHead);
  }
}

void RemoteLogger::broadcast(const uint8_t* data, size_t len) {
  for (auto& c : clients) {
    if (c.sock.connected()) c.sock.write(data, len);
  }
}
