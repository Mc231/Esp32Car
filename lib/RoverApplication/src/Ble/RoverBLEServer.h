#ifndef ROVERBLESERVER_H
#define ROVERBLESERVER_H

#ifdef ROVER_FEATURE_BLE

#include <Arduino.h>
#include <string>
#include "Command/CommandDispatcher.h"

// Forward declarations to keep NimBLE headers out of this header — they
// pull in a lot and slow down every TU that includes us.
class NimBLEServer;
class NimBLECharacteristic;

// BLE GATT peripheral that exposes the same JSON command surface as the
// WS server. Single service with two characteristics:
//   - CMD   (write)  — peer writes a JSON command frame
//   - STATE (notify) — peripheral notifies replies + telemetry
// Pairing is required (LE Legacy Pairing with static 6-digit passkey).
//
// UUIDs (custom 128-bit, random):
//   Service: 7c2d50c0-19a8-4a10-9c4b-ab5f7e3a2710
//   CMD    : 7c2d50c1-19a8-4a10-9c4b-ab5f7e3a2710
//   STATE  : 7c2d50c2-19a8-4a10-9c4b-ab5f7e3a2710
class RoverBLEServer {
public:
  RoverBLEServer(CommandDispatcher& dispatcher,
                 const std::string& deviceName,
                 const std::string& staticPasskey6);
  void begin();
  // Called by the CMD characteristic write callback.
  void handleIncoming(const std::string& payload);
  // Notify the current reply on STATE.
  void notifyReply(const std::string& payload);

  bool isInitialized() const { return initialized; }

private:
  CommandDispatcher& dispatcher;
  std::string deviceName;
  uint32_t passkey;
  NimBLEServer*         server   = nullptr;
  NimBLECharacteristic* cmdChar  = nullptr;
  NimBLECharacteristic* stateChar = nullptr;
  bool initialized = false;
};

#endif // ROVER_FEATURE_BLE
#endif // ROVERBLESERVER_H
