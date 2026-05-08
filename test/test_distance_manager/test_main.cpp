// Unit tests for DistanceManager. Native (host) build — drives
// `analogReadMilliVolts()` and `millis()` via the programmable globals
// in test/native_stubs/Arduino.h, then verifies the GP2Y0A21
// voltage→distance conversion, sample-interval gating, blind-zone
// folding, and out-of-range sentinel.
//
// platformio.ini sets -DUNITY_EXCLUDE_FLOAT, so we compare distances by
// scaling to integer centimetres (×10) — fine because the conversion
// curve has ~1cm precision in the operating range.

#include <unity.h>
#include <cmath>
#include <Arduino.h>

// Relative include — bypass the stub Manager/DistanceManager.h that
// CommandDispatcher tests rely on. The .cpp itself just resolves
// `Log/RemoteLogger.h` through the search path → stub.
#include "../../lib/RoverApplication/src/Manager/DistanceManager.h"
#include "../../lib/RoverApplication/src/Manager/DistanceManager.cpp"

namespace {

DistanceManager* mgr;

// Mirrors DistanceManager::SAMPLE_INTERVAL_MS — the production constant
// is private. If the production value changes, update here.
constexpr unsigned long SAMPLE_INTERVAL_MS = 50;

void setSensorMillivolts(uint32_t mv) { native_stub::g_analog_mv = mv; }
void setNow(unsigned long ms)         { native_stub::g_millis = ms; }

// Force a fresh sample (bypass the interval gate).
void forceSample() {
  native_stub::g_millis += SAMPLE_INTERVAL_MS + 1;
  mgr->update();
}

// Distances scaled to mm (×10) for integer assertions.
int distanceMm() { return static_cast<int>(mgr->getLastDistance() * 10.0f); }

} // namespace

void setUp(void) {
  native_stub::reset();
  mgr = new DistanceManager(33);
  mgr->initialize();
}

void tearDown(void) {
  delete mgr;
}

// ===== voltage→distance conversion =====

void test_far_object_low_voltage_marks_out_of_range(void) {
  // <0.4V means nothing within sensor's 80cm window — sentinel -1.
  setSensorMillivolts(300);
  forceSample();
  TEST_ASSERT_EQUAL_INT(-10, distanceMm());  // -1.0f * 10
}

void test_mid_range_voltage_yields_plausible_distance(void) {
  // ~1.3V on the GP2Y0A21 curve falls in the 20-30cm band.
  setSensorMillivolts(1300);
  forceSample();
  int mm = distanceMm();
  TEST_ASSERT_TRUE(mm >= 180 && mm <= 320);
}

void test_close_voltage_clamps_at_10cm_floor(void) {
  // GP2Y0A21 curve folds back inside 10cm; we treat that as the
  // bumper distance (10cm) rather than reporting nonsense.
  setSensorMillivolts(2900);
  forceSample();
  TEST_ASSERT_EQUAL_INT(100, distanceMm());  // 10.0cm * 10
}

void test_far_edge_voltage_yields_high_distance(void) {
  // 410mV sits just inside the 0.4V cutoff. The GP2Y0A21 curve gives
  // ~77.6cm at this voltage — well into the upper half of the range.
  setSensorMillivolts(410);
  forceSample();
  int mm = distanceMm();
  TEST_ASSERT_TRUE(mm >= 700 && mm <= 800);
}

// ===== sample interval gating =====

void test_update_within_interval_does_not_resample(void) {
  setSensorMillivolts(1500);
  setNow(0);
  mgr->update();                        // first sample at t=0
  int first = distanceMm();

  // Move sensor reading, but only advance time by less than the interval.
  setSensorMillivolts(2700);
  setNow(SAMPLE_INTERVAL_MS / 2);
  mgr->update();
  TEST_ASSERT_EQUAL_INT(first, distanceMm());
}

void test_update_after_interval_resamples(void) {
  setSensorMillivolts(1500);
  setNow(0);
  mgr->update();
  int first = distanceMm();

  setSensorMillivolts(2700);
  setNow(SAMPLE_INTERVAL_MS + 1);
  mgr->update();
  TEST_ASSERT_NOT_EQUAL(first, distanceMm());
}

// ===== state envelope =====

void test_state_exposes_last_distance(void) {
  setSensorMillivolts(1300);
  forceSample();
  auto state = mgr->getState();
  int mm = static_cast<int>(std::any_cast<float>(state.at("last_distance")) * 10.0f);
  TEST_ASSERT_TRUE(mm > 180 && mm < 320);
}

void test_state_carries_sentinel_when_out_of_range(void) {
  setSensorMillivolts(100);
  forceSample();
  auto state = mgr->getState();
  int mm = static_cast<int>(std::any_cast<float>(state.at("last_distance")) * 10.0f);
  TEST_ASSERT_EQUAL_INT(-10, mm);
}

int main(int /*argc*/, char** /*argv*/) {
  UNITY_BEGIN();
  RUN_TEST(test_far_object_low_voltage_marks_out_of_range);
  RUN_TEST(test_mid_range_voltage_yields_plausible_distance);
  RUN_TEST(test_close_voltage_clamps_at_10cm_floor);
  RUN_TEST(test_far_edge_voltage_yields_high_distance);
  RUN_TEST(test_update_within_interval_does_not_resample);
  RUN_TEST(test_update_after_interval_resamples);
  RUN_TEST(test_state_exposes_last_distance);
  RUN_TEST(test_state_carries_sentinel_when_out_of_range);
  return UNITY_END();
}
