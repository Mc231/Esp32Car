#ifdef ROVER_FEATURE_BLE

#include "RoverBLEServer.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include "Log/RemoteLogger.h"

namespace {
constexpr const char* kServiceUUID = "7c2d50c0-19a8-4a10-9c4b-ab5f7e3a2710";
constexpr const char* kCmdUUID     = "7c2d50c1-19a8-4a10-9c4b-ab5f7e3a2710";
constexpr const char* kStateUUID   = "7c2d50c2-19a8-4a10-9c4b-ab5f7e3a2710";

class CmdCallbacks : public NimBLECharacteristicCallbacks {
public:
  explicit CmdCallbacks(RoverBLEServer& owner) : owner(owner) {}
  void onWrite(NimBLECharacteristic* c) override {
    std::string val = c->getValue();
    if (val.empty()) return;
    owner.handleIncoming(val);
  }
private:
  RoverBLEServer& owner;
};

class ServerCallbacks : public NimBLEServerCallbacks {
public:
  void onConnect(NimBLEServer* s, ble_gap_conn_desc* desc) override {
    Log.printf("[ble] central connected, handle=%d\n", desc->conn_handle);
    // Negotiate higher MTU so single-shot replies fit (default 23B is too small).
    NimBLEDevice::setMTU(247);
  }
  void onDisconnect(NimBLEServer* s) override {
    Log.println("[ble] central disconnected, restarting advertising");
    NimBLEDevice::startAdvertising();
  }
  uint32_t onPassKeyRequest() override {
    // Only invoked if peer is the initiator and we are display-only;
    // returning the passkey here is unusual but harmless.
    return 0;
  }
  bool onConfirmPIN(uint32_t pin) override { return true; }
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    if (!desc->sec_state.encrypted) {
      Log.println("[ble] auth FAILED — disconnecting");
      NimBLEDevice::getServer()->disconnect(desc->conn_handle);
    } else {
      Log.println("[ble] auth OK");
    }
  }
};
} // namespace

RoverBLEServer::RoverBLEServer(CommandDispatcher& dispatcher,
                               const std::string& deviceName,
                               const std::string& staticPasskey6)
  : dispatcher(dispatcher), deviceName(deviceName), passkey(0) {
  // Convert the 6-digit string passkey to uint32_t (NimBLE expects an int 000000-999999).
  long n = staticPasskey6.empty() ? 0 : strtol(staticPasskey6.c_str(), nullptr, 10);
  if (n < 0 || n > 999999) n = 123456;
  passkey = static_cast<uint32_t>(n);
}

void RoverBLEServer::begin() {
  Log.printf("[ble] initializing as '%s' (pin %06u)\n", deviceName.c_str(), passkey);
  NimBLEDevice::init(deviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  // Static passkey, MITM, no bonding (no NVS state to leak between owners).
  NimBLEDevice::setSecurityAuth(/*bonding=*/false, /*MITM=*/true, /*SC=*/false);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityPasskey(passkey);

  server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = server->createService(kServiceUUID);

  cmdChar = svc->createCharacteristic(
    kCmdUUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN
  );
  cmdChar->setCallbacks(new CmdCallbacks(*this));

  stateChar = svc->createCharacteristic(
    kStateUUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN
  );
  stateChar->setValue(std::string("{\"status\":\"ready\"}"));

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(kServiceUUID);
  adv->setScanResponse(true);
  adv->setName(deviceName);
  NimBLEDevice::startAdvertising();
  initialized = true;
  Log.println("[ble] advertising started");
}

void RoverBLEServer::handleIncoming(const std::string& payload) {
  // Reply via STATE notify. Multiple respond() calls within one dispatch
  // will produce multiple notifications, in order — matches WS broadcast
  // semantics where each respond() is its own frame.
  dispatcher.dispatchRaw(payload, [this](const std::string& reply) {
    notifyReply(reply);
  });
}

void RoverBLEServer::notifyReply(const std::string& payload) {
  if (!stateChar) return;
  stateChar->setValue(payload);
  stateChar->notify();
}

#endif // ROVER_FEATURE_BLE
