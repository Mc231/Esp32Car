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
#include <algorithm>

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

// Programmable clock + ADC for tests. Tests poke these globals directly to
// drive time-sensitive logic (DistanceManager sample interval gating) and
// ADC readings (voltage→distance conversion). The functions below read
// them — tests just assign values before calling code under test.
namespace native_stub {
  inline unsigned long g_millis = 0;
  inline uint32_t      g_analog_mv = 0;
}

inline unsigned long millis() { return native_stub::g_millis; }
inline unsigned long micros() { return native_stub::g_millis * 1000UL; }
inline void delay(unsigned long) {}
inline uint32_t analogReadMilliVolts(int /*pin*/) { return native_stub::g_analog_mv; }

// Arduino's constrain() macro — used by MotorManager::changeSpeed.
#ifndef constrain
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#endif

// pinMode / digital / ledc — recordable for MotorManager-style tests.
namespace native_stub {
  struct PinEvent { int pin; int value; };
  inline std::string last_event;
  inline int last_pin_a = -1;
  inline int last_pin_b = -1;
  inline int last_pin_a_value = -1;
  inline int last_pin_b_value = -1;
  inline int last_ledc_channel = -1;
  inline uint32_t last_ledc_value = 0;
  inline void reset() {
    g_millis = 0; g_analog_mv = 0;
    last_event.clear();
    last_pin_a = last_pin_b = last_pin_a_value = last_pin_b_value = -1;
    last_ledc_channel = -1; last_ledc_value = 0;
  }
}

#define OUTPUT 1
#define INPUT  0
#define HIGH   1
#define LOW    0

inline void pinMode(int /*pin*/, int /*mode*/) {}
inline void digitalWrite(int pin, int value) {
  // Track in pair-order: first write seen → A, second → B. MotorManager
  // writes pinA then pinB on every action, so this is deterministic.
  if (native_stub::last_pin_a == -1 || native_stub::last_pin_a == pin) {
    native_stub::last_pin_a = pin; native_stub::last_pin_a_value = value;
  } else {
    native_stub::last_pin_b = pin; native_stub::last_pin_b_value = value;
  }
}
inline void ledcSetup(int /*chan*/, uint32_t /*freq*/, uint8_t /*bits*/) {}
inline void ledcAttachPin(int /*pin*/, int /*chan*/) {}
inline void ledcWrite(int chan, uint32_t value) {
  native_stub::last_ledc_channel = chan;
  native_stub::last_ledc_value = value;
}

#endif // NATIVE_ARDUINO_STUB_H
