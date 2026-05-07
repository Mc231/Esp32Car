// Minimal Arduino.h stub for native (host) PlatformIO unit tests.
// Provides just enough of the Arduino API surface that production code
// touched by tests can compile on the host. Real ESP32 builds use the
// full Arduino-ESP32 framework headers — this file is only on the
// `-I test/native_stubs` include path of the [env:native] environment.
#ifndef NATIVE_ARDUINO_STUB_H
#define NATIVE_ARDUINO_STUB_H

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

// Arduino's String — enough for our asJSON / config field uses.
class String {
public:
  String() = default;
  String(const char* s) : s_(s ? s : "") {}
  String(const std::string& s) : s_(s) {}
  const char* c_str() const { return s_.c_str(); }
  size_t length() const { return s_.size(); }
  bool isEmpty() const { return s_.empty(); }
  String& operator+=(const char* s) { s_ += s; return *this; }
  String& operator+=(const String& s) { s_ += s.s_; return *this; }
  String operator+(const char* s) const { return String(s_ + s); }
  bool operator==(const String& o) const { return s_ == o.s_; }
  std::string s_;
};

// Stubs for symbols Arduino code commonly references. Not all code paths
// using these will work on native — they're here so headers compile.
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void delay(unsigned long) {}

#endif // NATIVE_ARDUINO_STUB_H
