#include "RoverEspNowServer.h"
#include <esp_now.h>
#include <WiFi.h>
#include <cstring>
#include "Log/RemoteLogger.h"

namespace {
// ASCII "RVC0" — bumped if/when we change the wire format. Reading as
// little-endian uint32 = 0x30435652. We compare via raw memcmp on the
// first 4 bytes, so the on-wire layout is just the four ASCII chars.
constexpr uint8_t MAGIC[4] = { 'R', 'V', 'C', '0' };
constexpr uint8_t BROADCAST_MAC[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

struct RaiiLock {
  SemaphoreHandle_t& m;
  explicit RaiiLock(SemaphoreHandle_t& m) : m(m) { xSemaphoreTake(m, portMAX_DELAY); }
  ~RaiiLock() { xSemaphoreGive(m); }
};
} // namespace

RoverEspNowServer* RoverEspNowServer::instance = nullptr;

RoverEspNowServer::RoverEspNowServer(CommandDispatcher& dispatcher)
  : dispatcher(dispatcher), mu(xSemaphoreCreateMutex()) {}

RoverEspNowServer::~RoverEspNowServer() {
  if (running) stop();
  if (mu) vSemaphoreDelete(mu);
}

void RoverEspNowServer::begin() {
  if (running) return;
  if (esp_now_init() != ESP_OK) {
    Log.println("[espnow] esp_now_init failed");
    return;
  }
  // Add the broadcast peer so we can both send and receive on the
  // broadcast address. Channel 0 = use whatever Wi-Fi is currently on.
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Log.println("[espnow] esp_now_add_peer(broadcast) failed");
    esp_now_deinit();
    return;
  }

  instance = this;
  esp_now_register_recv_cb(&RoverEspNowServer::onRecvThunk);
  running = true;
  Log.printf("[espnow] started on Wi-Fi channel %d (broadcast)\n", WiFi.channel());
}

void RoverEspNowServer::stop() {
  if (!running) return;
  esp_now_unregister_recv_cb();
  esp_now_del_peer(BROADCAST_MAC);
  esp_now_deinit();
  instance = nullptr;
  running = false;
  // Drop any queued frames received before stop().
  RaiiLock lk(mu);
  incoming.clear();
  Log.println("[espnow] stopped");
}

void RoverEspNowServer::loop() {
  if (!running) return;
  std::vector<std::string> drained;
  {
    RaiiLock lk(mu);
    drained.swap(incoming);
  }
  for (auto& raw : drained) {
    dispatcher.dispatchRaw(raw, [this](const std::string& reply) {
      sendBroadcast(reply);
    });
  }
}

void RoverEspNowServer::onRecvThunk(const uint8_t* mac, const uint8_t* data, int len) {
  if (!instance) return;
  // Reject anything without our magic prefix — keeps unrelated ESP-NOW
  // traffic on the same channel from getting parsed as commands.
  if (len < (int)sizeof(MAGIC) || memcmp(data, MAGIC, sizeof(MAGIC)) != 0) return;
  instance->enqueue(data + sizeof(MAGIC), len - (int)sizeof(MAGIC));
}

void RoverEspNowServer::enqueue(const uint8_t* data, int len) {
  if (len <= 0) return;
  std::string s(reinterpret_cast<const char*>(data), len);
  RaiiLock lk(mu);
  incoming.push_back(std::move(s));
}

void RoverEspNowServer::sendBroadcast(const std::string& payload) {
  if (!running) return;
  if (payload.size() > MAX_PAYLOAD) {
    Log.printf("[espnow] reply %u B exceeds %u B max — dropping\n",
               (unsigned)payload.size(), (unsigned)MAX_PAYLOAD);
    return;
  }
  uint8_t buf[sizeof(MAGIC) + MAX_PAYLOAD];
  memcpy(buf, MAGIC, sizeof(MAGIC));
  memcpy(buf + sizeof(MAGIC), payload.data(), payload.size());
  esp_now_send(BROADCAST_MAC, buf, sizeof(MAGIC) + payload.size());
}
