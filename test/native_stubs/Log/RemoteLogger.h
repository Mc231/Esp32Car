// Native test stub for RemoteLogger. The real RemoteLogger drags in
// WiFi.h + ICommandTransport. For tests that only care about
// `Log.printf`/`Log.println`, this no-op stand-in is plenty.
#ifndef NATIVE_REMOTELOGGER_STUB_H
#define NATIVE_REMOTELOGGER_STUB_H

#include <cstdio>
#include <cstdarg>

class RemoteLoggerStub {
public:
  void println(const char* = "") {}
  int printf(const char*, ...) { return 0; }
  void print(const char*) {}
  void loop() {}
  void begin() {}
  void stop() {}
};

inline RemoteLoggerStub Log;

#endif
