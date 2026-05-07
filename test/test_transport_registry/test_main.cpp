// Unit tests for TransportRegistry. Native (host) build — no firmware,
// no Arduino framework, no hardware. Stubs are picked up via the
// `-I test/native_stubs` flag in [env:native].
#include <unity.h>
#include "Command/TransportRegistry.h"
#include "Command/TransportRegistry.cpp"   // direct include — single-binary test build

namespace {

// Minimal in-memory transport for testing the registry. Records its
// own lifecycle so tests can assert begin/stop/loop call counts.
class FakeTransport : public ICommandTransport {
public:
  explicit FakeTransport(const char* n) : nm(n) {}
  const char* name() const override { return nm; }
  void begin() override { running = true; beginCount++; }
  void stop()  override { running = false; stopCount++; }
  void loop()  override { loopCount++; }
  bool isRunning() const override { return running; }

  const char* nm;
  bool running = false;
  int beginCount = 0;
  int stopCount = 0;
  int loopCount = 0;
};

TransportRegistry* registry;
FakeTransport* http;
FakeTransport* ws;
FakeTransport* mqtt;

} // namespace

void setUp(void) {
  registry = new TransportRegistry();
  http = new FakeTransport("http");
  ws   = new FakeTransport("ws");
  mqtt = new FakeTransport("mqtt");
}

void tearDown(void) {
  delete registry;
  delete http; delete ws; delete mqtt;
}

void test_starts_empty(void) {
  TEST_ASSERT_EQUAL(0u, registry->size());
  TEST_ASSERT_NULL(registry->find("http"));
}

void test_add_increments_size(void) {
  registry->add(http);
  registry->add(ws);
  TEST_ASSERT_EQUAL(2u, registry->size());
}

void test_add_null_is_no_op(void) {
  registry->add(nullptr);
  TEST_ASSERT_EQUAL(0u, registry->size());
}

void test_find_by_name(void) {
  registry->add(http);
  registry->add(mqtt);
  TEST_ASSERT_EQUAL_PTR(http, registry->find("http"));
  TEST_ASSERT_EQUAL_PTR(mqtt, registry->find("mqtt"));
}

void test_find_unknown_returns_null(void) {
  registry->add(http);
  TEST_ASSERT_NULL(registry->find("ble"));
  TEST_ASSERT_NULL(registry->find(""));
}

void test_find_null_name_returns_null(void) {
  registry->add(http);
  TEST_ASSERT_NULL(registry->find(nullptr));
}

void test_at_returns_in_insertion_order(void) {
  registry->add(http);
  registry->add(ws);
  registry->add(mqtt);
  TEST_ASSERT_EQUAL_PTR(http, registry->at(0));
  TEST_ASSERT_EQUAL_PTR(ws,   registry->at(1));
  TEST_ASSERT_EQUAL_PTR(mqtt, registry->at(2));
}

void test_beginAll_starts_every_transport(void) {
  registry->add(http);
  registry->add(ws);
  registry->add(mqtt);
  registry->beginAll();
  TEST_ASSERT_TRUE(http->isRunning());
  TEST_ASSERT_TRUE(ws->isRunning());
  TEST_ASSERT_TRUE(mqtt->isRunning());
  TEST_ASSERT_EQUAL(1, http->beginCount);
}

void test_loopAll_pumps_every_transport(void) {
  registry->add(http);
  registry->add(ws);
  registry->loopAll();
  registry->loopAll();
  TEST_ASSERT_EQUAL(2, http->loopCount);
  TEST_ASSERT_EQUAL(2, ws->loopCount);
}

void test_stopAll_only_stops_running_transports(void) {
  registry->add(http);
  registry->add(ws);
  http->begin();   // only http is running
  registry->stopAll();
  TEST_ASSERT_EQUAL(1, http->stopCount);
  TEST_ASSERT_EQUAL(0, ws->stopCount);   // ws was never running, stop() not called
}

void test_stopAll_after_beginAll_stops_all(void) {
  registry->add(http);
  registry->add(ws);
  registry->beginAll();
  registry->stopAll();
  TEST_ASSERT_FALSE(http->isRunning());
  TEST_ASSERT_FALSE(ws->isRunning());
  TEST_ASSERT_EQUAL(1, http->stopCount);
  TEST_ASSERT_EQUAL(1, ws->stopCount);
}

int main(int /*argc*/, char** /*argv*/) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_empty);
  RUN_TEST(test_add_increments_size);
  RUN_TEST(test_add_null_is_no_op);
  RUN_TEST(test_find_by_name);
  RUN_TEST(test_find_unknown_returns_null);
  RUN_TEST(test_find_null_name_returns_null);
  RUN_TEST(test_at_returns_in_insertion_order);
  RUN_TEST(test_beginAll_starts_every_transport);
  RUN_TEST(test_loopAll_pumps_every_transport);
  RUN_TEST(test_stopAll_only_stops_running_transports);
  RUN_TEST(test_stopAll_after_beginAll_stops_all);
  return UNITY_END();
}
