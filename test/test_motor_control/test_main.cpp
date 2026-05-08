// Unit tests for MotorControl. Native (host) build — uses the stub
// MotorManager from test/native_stubs/Manager/MotorManager.h to record
// call routing (left/right/both) and PWM values without LEDC hardware.
//
// We bypass the include path lookup for MotorControl.h with a relative
// include so we get the REAL header (the stub directory contains a
// minimal MotorControl.h shadow used by CommandDispatcher tests).

#include <unity.h>
#include <any>
#include <map>
#include <string>

#include "../../lib/RoverApplication/src/Control/MotorControl.h"
#include "../../lib/RoverApplication/src/Control/MotorControl.cpp"

namespace {

MotorManager* leftMotor;
MotorManager* rightMotor;
MotorControl* control;

template <typename T>
T as(const std::map<std::string, std::any>& m, const char* k) {
  return std::any_cast<T>(m.at(k));
}

} // namespace

void setUp(void) {
  leftMotor  = new MotorManager(1, 2, 3, 4);
  rightMotor = new MotorManager(5, 6, 7, 5);
  control    = new MotorControl(*leftMotor, *rightMotor);
}

void tearDown(void) {
  delete control;
  delete rightMotor;
  delete leftMotor;
}

// ===== begin() =====

void test_begin_initialises_both_motors(void) {
  control->begin();
  TEST_ASSERT_EQUAL(1, leftMotor->beginCalls);
  TEST_ASSERT_EQUAL(1, rightMotor->beginCalls);
}

// ===== action() routing =====

void test_action_left_only_routes_to_left(void) {
  control->action(FORWARD, LEFT);
  TEST_ASSERT_EQUAL(1, leftMotor->actionCalls);
  TEST_ASSERT_EQUAL(0, rightMotor->actionCalls);
  TEST_ASSERT_EQUAL(FORWARD, leftMotor->currentAction);
}

void test_action_right_only_routes_to_right(void) {
  control->action(BACKWARD, RIGHT);
  TEST_ASSERT_EQUAL(0, leftMotor->actionCalls);
  TEST_ASSERT_EQUAL(1, rightMotor->actionCalls);
  TEST_ASSERT_EQUAL(BACKWARD, rightMotor->currentAction);
}

void test_action_all_routes_to_both(void) {
  control->action(FORWARD, ALL);
  TEST_ASSERT_EQUAL(1, leftMotor->actionCalls);
  TEST_ASSERT_EQUAL(1, rightMotor->actionCalls);
  TEST_ASSERT_EQUAL(FORWARD, leftMotor->currentAction);
  TEST_ASSERT_EQUAL(FORWARD, rightMotor->currentAction);
}

void test_action_stop_propagates_to_both(void) {
  control->action(FORWARD, ALL);
  control->action(STOP, ALL);
  TEST_ASSERT_EQUAL(STOP, leftMotor->currentAction);
  TEST_ASSERT_EQUAL(STOP, rightMotor->currentAction);
}

// ===== setPWM() routing =====

void test_setPWM_left_only(void) {
  control->setPWM(LEFT, 200);
  TEST_ASSERT_EQUAL(1, leftMotor->speedCalls);
  TEST_ASSERT_EQUAL(0, rightMotor->speedCalls);
  TEST_ASSERT_EQUAL(200, leftMotor->currentSpeed);
}

void test_setPWM_right_only(void) {
  control->setPWM(RIGHT, 175);
  TEST_ASSERT_EQUAL(0, leftMotor->speedCalls);
  TEST_ASSERT_EQUAL(1, rightMotor->speedCalls);
  TEST_ASSERT_EQUAL(175, rightMotor->currentSpeed);
}

void test_setPWM_all(void) {
  control->setPWM(ALL, 128);
  TEST_ASSERT_EQUAL(128, leftMotor->currentSpeed);
  TEST_ASSERT_EQUAL(128, rightMotor->currentSpeed);
}

// ===== getState() =====

void test_state_reports_actions_and_speeds(void) {
  control->action(FORWARD, ALL);
  control->setPWM(ALL, 200);
  auto state = control->getState();
  TEST_ASSERT_EQUAL(static_cast<int>(FORWARD), as<int>(state, "lma"));
  TEST_ASSERT_EQUAL(static_cast<int>(FORWARD), as<int>(state, "rma"));
  TEST_ASSERT_EQUAL(200, as<int>(state, "lmpwm"));
  TEST_ASSERT_EQUAL(200, as<int>(state, "rmpwm"));
}

void test_state_remembers_last_selection(void) {
  control->action(FORWARD, LEFT);
  control->setPWM(RIGHT, 90);
  auto state = control->getState();
  TEST_ASSERT_EQUAL(static_cast<int>(LEFT),  as<int>(state, "las"));
  TEST_ASSERT_EQUAL(static_cast<int>(RIGHT), as<int>(state, "lpwms"));
}

void test_state_reflects_independent_left_right(void) {
  control->action(FORWARD,  LEFT);
  control->action(BACKWARD, RIGHT);
  control->setPWM(LEFT,  100);
  control->setPWM(RIGHT, 250);
  auto state = control->getState();
  TEST_ASSERT_EQUAL(static_cast<int>(FORWARD),  as<int>(state, "lma"));
  TEST_ASSERT_EQUAL(static_cast<int>(BACKWARD), as<int>(state, "rma"));
  TEST_ASSERT_EQUAL(100, as<int>(state, "lmpwm"));
  TEST_ASSERT_EQUAL(250, as<int>(state, "rmpwm"));
}

int main(int /*argc*/, char** /*argv*/) {
  UNITY_BEGIN();
  RUN_TEST(test_begin_initialises_both_motors);
  RUN_TEST(test_action_left_only_routes_to_left);
  RUN_TEST(test_action_right_only_routes_to_right);
  RUN_TEST(test_action_all_routes_to_both);
  RUN_TEST(test_action_stop_propagates_to_both);
  RUN_TEST(test_setPWM_left_only);
  RUN_TEST(test_setPWM_right_only);
  RUN_TEST(test_setPWM_all);
  RUN_TEST(test_state_reports_actions_and_speeds);
  RUN_TEST(test_state_remembers_last_selection);
  RUN_TEST(test_state_reflects_independent_left_right);
  return UNITY_END();
}
