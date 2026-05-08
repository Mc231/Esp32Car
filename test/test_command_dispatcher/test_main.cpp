// Unit tests for CommandDispatcher. Native (host) build — uses stubs from
// test/native_stubs/ so production headers (RoverController, SystemMonitor,
// Log) resolve to in-memory fakes free of FreeRTOS/Arduino deps.
//
// The dispatcher is the heart of the unified command surface — every
// transport (HTTP, WS, Telnet, Serial, ESP-NOW, MQTT) routes through
// dispatchRaw(). Tests here cover routing, parsing, error paths, and
// reply shape for every command in the protocol.

#include <cstdio>
#include <string>
#include <cmath>
#include <unity.h>

#include "Config/RoverApplicationConfig.h"
#include "Command/CommandDispatcher.h"
#include "Command/CommandDispatcher.cpp"
#include "Command/TransportRegistry.cpp"

namespace {

// Last reply captured by the most recent dispatchRaw() call. Tests
// inspect this rather than wrangling lambdas inline.
std::string lastReply;
int replyCount = 0;
auto captureReply = [](const std::string& reply) {
  lastReply = reply;
  replyCount++;
};

RoverController* controller;
SystemMonitor* monitor;
RoverApplicationConfig* config;
CommandDispatcher* dispatcher;

bool replyContains(const char* needle) {
  return lastReply.find(needle) != std::string::npos;
}

} // namespace

void setUp(void) {
  controller = new RoverController();
  monitor = new SystemMonitor();
  config = new RoverApplicationConfig();
  dispatcher = new CommandDispatcher(*controller, *config, *monitor);
  lastReply = "";
  replyCount = 0;
}

void tearDown(void) {
  delete dispatcher; delete config; delete monitor; delete controller;
}

// ===== Parser & error handling =====

void test_invalid_json_no_reply(void) {
  dispatcher->dispatchRaw("not json", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_missing_command_field_no_reply(void) {
  dispatcher->dispatchRaw("{\"foo\":1}", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_unknown_command_no_reply(void) {
  dispatcher->dispatchRaw("{\"command\":\"nope\"}", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

// ===== system / status / config / wifi / distance =====

void test_system_returns_response_with_monitor_state(void) {
  monitor->state["up_time"] = 42;
  dispatcher->dispatchRaw("{\"command\":\"system\"}", captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("\"response\""));
  TEST_ASSERT_TRUE(replyContains("\"up_time\":42"));
}

void test_status_includes_motor_and_system(void) {
  controller->motorState["lma"] = 0;
  monitor->state["free_heap"] = 188432;
  dispatcher->dispatchRaw("{\"command\":\"status\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("\"motor\""));
  TEST_ASSERT_TRUE(replyContains("\"system\""));
  TEST_ASSERT_TRUE(replyContains("188432"));
}

void test_config_response_uses_camelCase_keys(void) {
  dispatcher->dispatchRaw("{\"command\":\"config\"}", captureReply);
  // Regression: the field used to be `distance_sensor_enabled` (snake)
  // while neighbours were camel — now must be camelCase consistently.
  TEST_ASSERT_TRUE(replyContains("\"distanceSensorEnabled\""));
  TEST_ASSERT_TRUE(replyContains("\"leftMotorPin1\""));
  TEST_ASSERT_TRUE(replyContains("\"distanceSensorPin\""));
}

void test_wifi_replies_with_controller_config(void) {
  controller->wiFiConfig["ssid"] = std::string("homenet");
  controller->wiFiConfig["ip"] = std::string("192.168.1.42");
  dispatcher->dispatchRaw("{\"command\":\"wifi\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("homenet"));
  TEST_ASSERT_TRUE(replyContains("192.168.1.42"));
}

void test_distance_returns_response_when_feature_enabled(void) {
#ifdef ROVER_FEATURE_DISTANCE
  controller->distanceState["last_distance"] = 35;
  dispatcher->dispatchRaw("{\"command\":\"distance\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("\"response\""));
  TEST_ASSERT_TRUE(replyContains("35"));
#else
  dispatcher->dispatchRaw("{\"command\":\"distance\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("not built"));
#endif
}

// ===== motor commands =====

void test_motor_state_command(void) {
  controller->motorState["lmpwm"] = 200;
  dispatcher->dispatchRaw("{\"command\":\"motor_state\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("\"response\""));
  TEST_ASSERT_TRUE(replyContains("200"));
}

void test_set_motor_routes_to_controller(void) {
  dispatcher->dispatchRaw("{\"command\":\"set_motor\",\"action\":0,\"motor\":2}", captureReply);
  TEST_ASSERT_TRUE(controller->setMotorActionCalled);
  TEST_ASSERT_EQUAL(FORWARD, controller->lastSetAction);
  TEST_ASSERT_EQUAL(BOTH_MOTORS, controller->lastSetActionMotor);
}

void test_set_motor_replies_with_motor_state(void) {
  controller->motorState["lma"] = 0;
  dispatcher->dispatchRaw("{\"command\":\"set_motor\",\"action\":0,\"motor\":2}", captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("\"response\""));
}

void test_set_motor_missing_args_returns_status_error(void) {
  dispatcher->dispatchRaw("{\"command\":\"set_motor\",\"action\":0}", captureReply);
  TEST_ASSERT_TRUE(replyContains("not passed"));
  TEST_ASSERT_FALSE(controller->setMotorActionCalled);
}

void test_set_motor_pwm_routes_to_controller(void) {
  dispatcher->dispatchRaw("{\"command\":\"set_motor_pwm\",\"motor\":1,\"pwm\":180}", captureReply);
  TEST_ASSERT_TRUE(controller->setMotorSpeedCalled);
  TEST_ASSERT_EQUAL(180, controller->lastSetSpeed);
  TEST_ASSERT_EQUAL(RIGHT_MOTOR, controller->lastSetSpeedMotor);
}

void test_set_motor_pwm_missing_args_returns_status_error(void) {
  dispatcher->dispatchRaw("{\"command\":\"set_motor_pwm\",\"motor\":2}", captureReply);
  TEST_ASSERT_TRUE(replyContains("not passed"));
  TEST_ASSERT_FALSE(controller->setMotorSpeedCalled);
}

// ===== reboot / forget_wi_fi =====

void test_reboot_replies_then_calls_reboot(void) {
  dispatcher->dispatchRaw("{\"command\":\"reboot\"}", captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("rebooting"));
  TEST_ASSERT_EQUAL(1, controller->rebootCount);
}

void test_forget_wifi_clears_then_reboots(void) {
  dispatcher->dispatchRaw("{\"command\":\"forget_wi_fi\"}", captureReply);
  TEST_ASSERT_EQUAL(1, controller->forgotWiFiCount);
  TEST_ASSERT_EQUAL(1, controller->rebootCount);
}

// ===== set_camera =====

void test_set_camera_missing_frame_size(void) {
  dispatcher->dispatchRaw("{\"command\":\"set_camera\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("missing"));
}

void test_set_camera_with_frame_size_sensor_null(void) {
  // Our esp_camera stub returns null sensor — dispatcher should
  // gracefully report "not initialized".
  dispatcher->dispatchRaw("{\"command\":\"set_camera\",\"frame_size\":5}", captureReply);
  TEST_ASSERT_TRUE(replyContains("not initialized"));
}

// ===== telemetry helper =====

void test_buildTelemetry_emits_envelope(void) {
  monitor->state["up_time"] = 7;
  controller->motorState["lma"] = 1;
  std::string payload = dispatcher->buildTelemetry();
  TEST_ASSERT_TRUE(payload.find("\"telemetry\"") != std::string::npos);
  TEST_ASSERT_TRUE(payload.find("\"system\"") != std::string::npos);
  TEST_ASSERT_TRUE(payload.find("\"motor\"") != std::string::npos);
}

// ===== transport-toggle commands =====

void test_transports_without_registry_returns_unavailable(void) {
  dispatcher->dispatchRaw("{\"command\":\"transports\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("unavailable"));
}

class StubTransport : public ICommandTransport {
public:
  explicit StubTransport(const char* n) : nm(n) {}
  const char* name() const override { return nm; }
  void begin() override { running = true; }
  void stop()  override { running = false; }
  bool isRunning() const override { return running; }
  const char* nm;
  bool running = false;
};

void test_transports_lists_registered_with_state(void) {
  TransportRegistry reg;
  StubTransport http("http");
  StubTransport mqtt("mqtt");
  http.running = true;
  reg.add(&http);
  reg.add(&mqtt);
  dispatcher->setTransportRegistry(&reg);
  dispatcher->dispatchRaw("{\"command\":\"transports\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("\"name\":\"http\""));
  TEST_ASSERT_TRUE(replyContains("\"running\":true"));
  TEST_ASSERT_TRUE(replyContains("\"name\":\"mqtt\""));
  TEST_ASSERT_TRUE(replyContains("\"running\":false"));
}

void test_set_transport_starts_target(void) {
  TransportRegistry reg;
  StubTransport mqtt("mqtt");
  reg.add(&mqtt);
  dispatcher->setTransportRegistry(&reg);
  dispatcher->dispatchRaw(
    "{\"command\":\"set_transport\",\"name\":\"mqtt\",\"enabled\":true}",
    captureReply);
  TEST_ASSERT_TRUE(mqtt.running);
}

void test_set_transport_stops_target(void) {
  TransportRegistry reg;
  StubTransport mqtt("mqtt");
  mqtt.running = true;
  reg.add(&mqtt);
  dispatcher->setTransportRegistry(&reg);
  dispatcher->dispatchRaw(
    "{\"command\":\"set_transport\",\"name\":\"mqtt\",\"enabled\":false}",
    captureReply);
  TEST_ASSERT_FALSE(mqtt.running);
}

void test_set_transport_unknown_name(void) {
  TransportRegistry reg;
  dispatcher->setTransportRegistry(&reg);
  dispatcher->dispatchRaw(
    "{\"command\":\"set_transport\",\"name\":\"xyz\",\"enabled\":true}",
    captureReply);
  TEST_ASSERT_TRUE(replyContains("unknown"));
}

void test_set_transport_missing_fields(void) {
  TransportRegistry reg;
  dispatcher->setTransportRegistry(&reg);
  dispatcher->dispatchRaw(
    "{\"command\":\"set_transport\",\"name\":\"mqtt\"}", captureReply);
  TEST_ASSERT_TRUE(replyContains("missing"));
}

// ===== JSON parsing edge cases =====

void test_empty_string_no_reply(void) {
  dispatcher->dispatchRaw("", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_empty_object_no_reply(void) {
  dispatcher->dispatchRaw("{}", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_truncated_json_no_reply(void) {
  // Half-formed payload — common when a transport buffers and a
  // newline arrives mid-message.
  dispatcher->dispatchRaw("{\"command\":\"system", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_command_with_leading_and_trailing_whitespace(void) {
  // Telnet/serial often include trailing CR/LF — the JSON parser is
  // tolerant of surrounding whitespace, so this still routes to `system`.
  dispatcher->dispatchRaw("  \r\n  {\"command\":\"system\"}  \r\n",
                          captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("\"response\""));
}

void test_extra_unknown_fields_are_ignored(void) {
  // Forward-compat: extra keys must not break dispatch — the contract
  // is "ignore what you don't understand" so older firmware can talk
  // to newer clients.
  dispatcher->dispatchRaw(
    "{\"command\":\"system\",\"future_field\":42,\"nested\":{\"x\":1}}",
    captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("\"response\""));
}

void test_set_motor_with_string_action_rejected(void) {
  // Type mismatch — `action` must be int. Without the type guard,
  // nlohmann::json throws on `.get<int>()` and aborts the process.
  dispatcher->dispatchRaw(
    "{\"command\":\"set_motor\",\"action\":\"forward\",\"motor\":2}",
    captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("must be integers"));
  TEST_ASSERT_FALSE(controller->setMotorActionCalled);
}

void test_set_motor_pwm_with_string_pwm_rejected(void) {
  dispatcher->dispatchRaw(
    "{\"command\":\"set_motor_pwm\",\"motor\":1,\"pwm\":\"fast\"}",
    captureReply);
  TEST_ASSERT_EQUAL(1, replyCount);
  TEST_ASSERT_TRUE(replyContains("must be integers"));
  TEST_ASSERT_FALSE(controller->setMotorSpeedCalled);
}

void test_command_inside_nested_object_is_ignored(void) {
  // Only the top-level `command` field is honored.
  dispatcher->dispatchRaw(
    "{\"wrapper\":{\"command\":\"system\"}}", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_command_with_numeric_value_no_reply(void) {
  // `command` must be a string; numbers are not a valid command name.
  dispatcher->dispatchRaw("{\"command\":42}", captureReply);
  TEST_ASSERT_EQUAL(0, replyCount);
}

void test_two_back_to_back_dispatches_each_reply(void) {
  // Each transport line is one envelope — verify we don't carry parser
  // state between calls.
  dispatcher->dispatchRaw("{\"command\":\"system\"}", captureReply);
  dispatcher->dispatchRaw("{\"command\":\"status\"}", captureReply);
  TEST_ASSERT_EQUAL(2, replyCount);
}

int main(int /*argc*/, char** /*argv*/) {
  UNITY_BEGIN();
  // Parser & error paths
  RUN_TEST(test_invalid_json_no_reply);
  RUN_TEST(test_missing_command_field_no_reply);
  RUN_TEST(test_unknown_command_no_reply);
  // Read-only commands
  RUN_TEST(test_system_returns_response_with_monitor_state);
  RUN_TEST(test_status_includes_motor_and_system);
  RUN_TEST(test_config_response_uses_camelCase_keys);
  RUN_TEST(test_wifi_replies_with_controller_config);
  RUN_TEST(test_distance_returns_response_when_feature_enabled);
  RUN_TEST(test_motor_state_command);
  // Mutation commands
  RUN_TEST(test_set_motor_routes_to_controller);
  RUN_TEST(test_set_motor_replies_with_motor_state);
  RUN_TEST(test_set_motor_missing_args_returns_status_error);
  RUN_TEST(test_set_motor_pwm_routes_to_controller);
  RUN_TEST(test_set_motor_pwm_missing_args_returns_status_error);
  RUN_TEST(test_reboot_replies_then_calls_reboot);
  RUN_TEST(test_forget_wifi_clears_then_reboots);
  // Camera
  RUN_TEST(test_set_camera_missing_frame_size);
  RUN_TEST(test_set_camera_with_frame_size_sensor_null);
  // Telemetry helper
  RUN_TEST(test_buildTelemetry_emits_envelope);
  // Transport-toggle commands
  RUN_TEST(test_transports_without_registry_returns_unavailable);
  RUN_TEST(test_transports_lists_registered_with_state);
  RUN_TEST(test_set_transport_starts_target);
  RUN_TEST(test_set_transport_stops_target);
  RUN_TEST(test_set_transport_unknown_name);
  RUN_TEST(test_set_transport_missing_fields);
  // JSON parsing edge cases
  RUN_TEST(test_empty_string_no_reply);
  RUN_TEST(test_empty_object_no_reply);
  RUN_TEST(test_truncated_json_no_reply);
  RUN_TEST(test_command_with_leading_and_trailing_whitespace);
  RUN_TEST(test_extra_unknown_fields_are_ignored);
  RUN_TEST(test_set_motor_with_string_action_rejected);
  RUN_TEST(test_set_motor_pwm_with_string_pwm_rejected);
  RUN_TEST(test_command_inside_nested_object_is_ignored);
  RUN_TEST(test_command_with_numeric_value_no_reply);
  RUN_TEST(test_two_back_to_back_dispatches_each_reply);
  return UNITY_END();
}
