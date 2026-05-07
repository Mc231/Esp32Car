#ifndef ICOMMANDTRANSPORT_H
#define ICOMMANDTRANSPORT_H

// Common interface for any transport that carries the unified JSON
// command surface (HTTP, WebSocket, MQTT, …).
//
// Lifecycle:
//   begin() is called when the transport should start serving requests
//          (typically right after Wi-Fi is up).
//   stop()  cleanly shuts it down so it can be brought up again later.
//   loop()  is pumped from the main loop; transports that run their own
//          FreeRTOS task (e.g. NimBLE) leave the default no-op.
//   isRunning() reflects current state — used by the registry and by the
//          `transports` dispatcher command to report status.
//
// Every implementer constructs a reply lambda that pipes the dispatcher's
// JSON reply back over its own pipe (broadcastTXT / notify / publish /
// HTTP send) and calls dispatcher.dispatchRaw(raw, replyFn) for each
// incoming frame. The dispatcher itself never knows which transport
// called it.
class ICommandTransport {
public:
  virtual ~ICommandTransport() = default;

  // A short identifier used in logs and the `set_transport` command.
  // Stable, lowercase, no spaces (e.g. "http", "ws", "mqtt").
  virtual const char* name() const = 0;

  // Start the transport. Idempotent — calling begin() while already
  // running should be a safe no-op.
  virtual void begin() = 0;

  // Stop the transport cleanly. Idempotent. After stop(), begin() should
  // bring it back up to the same configuration.
  virtual void stop() = 0;

  // Pump from the main loop. Default is a no-op for transports that
  // run their own task.
  virtual void loop() {}

  virtual bool isRunning() const = 0;
};

#endif // ICOMMANDTRANSPORT_H
