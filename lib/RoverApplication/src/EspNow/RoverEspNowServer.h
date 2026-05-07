#ifndef ROVERESPNOWSERVER_H
#define ROVERESPNOWSERVER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>
#include <string>
#include "Command/CommandDispatcher.h"
#include "Command/ICommandTransport.h"

// ESP-NOW peer-to-peer transport for the unified JSON command surface.
//
// Why this exists: ESP-NOW works without a Wi-Fi router or IP stack —
// great for a phone-free remote (another ESP32 + joystick) or for
// rover-to-rover swarm experiments. Range ~200 m line of sight.
//
// Wire format: each ESP-NOW frame is `[4-byte magic 'RVC0'][JSON bytes]`.
// The magic prefix lets us ignore other ESP-NOW traffic on the same
// channel (other projects, library demos, etc.) and lets remote clients
// recognise our replies. JSON payload limited to 246 B per frame
// (250 B ESP-NOW max minus the 4-byte magic).
//
// Pairing: receive on the broadcast address; send replies as broadcast
// too. No authentication. v1 trade-off — simpler, every device in range
// can drive. Channel must match the rover's current Wi-Fi channel; the
// rover follows whatever channel its STA AP is on.
class RoverEspNowServer : public ICommandTransport {
public:
  explicit RoverEspNowServer(CommandDispatcher& dispatcher);
  ~RoverEspNowServer() override;

  // ICommandTransport
  const char* name() const override { return "espnow"; }
  void begin() override;
  void stop() override;
  void loop() override;
  bool isRunning() const override { return running; }

  // Maximum JSON payload bytes per frame (ESP-NOW caps at 250 minus our
  // 4-byte magic).
  static constexpr size_t MAX_PAYLOAD = 246;

private:
  CommandDispatcher& dispatcher;
  bool running = false;

  // Receive callback fires in WiFi-task context; we enqueue here and
  // drain in loop() to avoid running the dispatcher off-task.
  std::vector<std::string> incoming;
  SemaphoreHandle_t mu;

  static RoverEspNowServer* instance;   // for the static recv thunk
  static void onRecvThunk(const uint8_t* mac, const uint8_t* data, int len);

  void enqueue(const uint8_t* data, int len);
  void sendBroadcast(const std::string& payload);
};

#endif // ROVERESPNOWSERVER_H
