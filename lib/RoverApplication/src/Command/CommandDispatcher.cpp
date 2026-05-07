#include "CommandDispatcher.h"
#include "esp_camera.h"
#include "Log/RemoteLogger.h"

using json = nlohmann::json;

CommandDispatcher::CommandDispatcher(RoverController& controller,
                                     const RoverApplicationConfig& config,
                                     SystemMonitor& systemMonitor)
  : controller(controller), config(config), systemMonitor(systemMonitor) {}

void CommandDispatcher::dispatchRaw(const std::string& raw, const ReplyFn& respond) {
  json j = json::parse(raw, nullptr, false);
  if (j.is_discarded()) {
    Log.println("CommandDispatcher: invalid JSON");
    return;
  }
  if (!j.contains("command")) {
    Log.println("CommandDispatcher: no command specified");
    return;
  }
  const std::string command = j["command"].get<std::string>();

  if (command == "system")           handleSystem(respond);
  else if (command == "status")      handleStatus(respond);
  else if (command == "config")      handleConfig(respond);
  else if (command == "reboot")      handleReboot(respond);
  else if (command == "wifi")        handleGetWiFi(respond);
  else if (command == "forget_wi_fi") handleWiFiForget(respond);
  else if (command == "distance")    handleGetDistance(respond);
  else if (command == "set_camera")  handleCamera(j, respond);
  else if (command == "motor_state") handleMotorState(respond);
  else if (command == "set_motor")   handleSetMotor(j, respond);
  else if (command == "set_motor_pwm") handleSetMotorPWM(j, respond);
  else if (command == "transports")    handleListTransports(respond);
  else if (command == "set_transport") handleSetTransport(j, respond);
}

void CommandDispatcher::handleListTransports(const ReplyFn& respond) {
  if (!registry) {
    respond("{\"status\":\"transport registry unavailable\"}");
    return;
  }
  // Build the response by hand — transports list is a JSON array of
  // {name, running} objects which doesn't fit cleanly into the std::any
  // map shape.
  std::string out = "{\"response\":[";
  for (size_t i = 0; i < registry->size(); ++i) {
    auto* t = registry->at(i);
    if (i) out += ",";
    out += "{\"name\":\"";
    out += t->name();
    out += "\",\"running\":";
    out += t->isRunning() ? "true" : "false";
    out += "}";
  }
  out += "]}";
  respond(out);
}

void CommandDispatcher::handleSetTransport(const json& j, const ReplyFn& respond) {
  if (!registry) {
    respond("{\"status\":\"transport registry unavailable\"}");
    return;
  }
  if (!j.contains("name") || !j.contains("enabled")) {
    respond("{\"status\":\"name or enabled missing\"}");
    return;
  }
  std::string n = j["name"].get<std::string>();
  bool enabled  = j["enabled"].get<bool>();
  auto* t = registry->find(n.c_str());
  if (!t) {
    respond("{\"status\":\"unknown transport\"}");
    return;
  }
  if (enabled) t->begin(); else t->stop();
  // Toggle is in-memory only — boot defaults come from RoverApplicationConfig
  // (set in main.cpp). To persist a new boot default, edit the config there.

  std::string out = "{\"response\":{\"name\":\"";
  out += n;
  out += "\",\"running\":";
  out += t->isRunning() ? "true" : "false";
  out += "}}";
  respond(out);
}

std::string CommandDispatcher::buildTelemetry() {
  std::map<std::string, std::any> result;
  result["system"]   = systemMonitor.getState();
  result["motor"]    = controller.getMotorState();
#ifdef ROVER_FEATURE_DISTANCE
  result["distance"] = controller.getDistanceState();
#endif
  std::map<std::string, std::any> envelope = {{"telemetry", result}};
  return serialize(envelope);
}

void CommandDispatcher::handleSystem(const ReplyFn& respond) {
  std::map<std::string, std::any> dataMap = {{"response", systemMonitor.getState()}};
  respond(serialize(dataMap));
}

void CommandDispatcher::handleStatus(const ReplyFn& respond) {
  std::map<std::string, std::any> result;
  result["motor"]  = controller.getMotorState();
  result["system"] = systemMonitor.getState();
  std::map<std::string, std::any> dataMap = {{"response", result}};
  respond(serialize(dataMap));
}

void CommandDispatcher::handleConfig(const ReplyFn& respond) {
  std::map<std::string, std::any> result;
  result["leftMotorPin1"]            = config.leftMotorPin1;
  result["leftMotorPin2"]            = config.leftMotorPin2;
  result["leftMotorPwm"]             = config.leftMotorPwm;
  result["rightMotorPin1"]           = config.rightMotorPin1;
  result["rightMotorPin2"]           = config.rightMotorPin2;
  result["rightMotorPwm"]            = config.rightMotorPwm;
  result["distanceSensorPin"]      = config.distanceSensorPin;
#ifdef ROVER_FEATURE_DISTANCE
  result["distanceSensorEnabled"]  = true;
#else
  result["distanceSensorEnabled"]  = false;
#endif
  std::map<std::string, std::any> dataMap = {{"response", result}};
  respond(serialize(dataMap));
}

void CommandDispatcher::handleReboot(const ReplyFn& respond) {
  std::map<std::string, std::any> dataMap = {{"status", std::string("rebooting")}};
  respond(serialize(dataMap));
  controller.reboot();
}

void CommandDispatcher::handleGetWiFi(const ReplyFn& respond) {
  auto wifi = controller.getWiFiConfig();
  std::map<std::string, std::any> dataMap = {{"response", wifi}};
  respond(serialize(dataMap));
}

void CommandDispatcher::handleWiFiForget(const ReplyFn& respond) {
  controller.forgotWiFi();
  std::map<std::string, std::any> dataMap = {{"status", std::string("Forget WI-FI Success")}};
  respond(serialize(dataMap));
  controller.reboot();
}

void CommandDispatcher::handleGetDistance(const ReplyFn& respond) {
#ifdef ROVER_FEATURE_DISTANCE
  std::map<std::string, std::any> dataMap = {{"response", controller.getDistanceState()}};
  respond(serialize(dataMap));
#else
  respond("{\"status\":\"distance sensor not built into this firmware\"}");
#endif
}

void CommandDispatcher::handleCamera(const json& j, const ReplyFn& respond) {
  if (!j.contains("frame_size")) {
    std::map<std::string, std::any> result = {{"status", std::string("Frame size parameter missing")}};
    respond(serialize(result));
    return;
  }
  int size = j["frame_size"].get<int>();
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, (framesize_t)size);
    respond("{\"status\": \"Camera frame size updated\"}");
  } else {
    respond("{\"status\": \"Camera not initialized\"}");
  }
}

void CommandDispatcher::handleSetMotorPWM(const json& j, const ReplyFn& respond) {
  if (!j.contains("pwm") || !j.contains("motor")) {
    respond("{\"status\": \"pwm or motor is not passed\"}");
    return;
  }
  int motor = j["motor"].get<int>();
  MotorSelection selection = static_cast<MotorSelection>(motor);
  int pwmValue = j["pwm"].get<int>();
  controller.setMotorSpeed(selection, pwmValue);
  handleMotorState(respond);
}

void CommandDispatcher::handleMotorState(const ReplyFn& respond) {
  std::map<std::string, std::any> dataMap = {{"response", controller.getMotorState()}};
  respond(serialize(dataMap));
}

void CommandDispatcher::handleSetMotor(const json& j, const ReplyFn& respond) {
  if (!j.contains("action") || !j.contains("motor")) {
    respond("{\"status\": \"Action or motor is not passed\"}");
    return;
  }
  MotorAction action = static_cast<MotorAction>(j["action"].get<int>());
  MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
  controller.setMotorAction(action, selection);
  handleMotorState(respond);
}

std::string CommandDispatcher::serialize(const std::map<std::string, std::any>& dataMap) const {
  return asJSON(dataMap).dump();
}

json CommandDispatcher::asJSON(const std::map<std::string, std::any>& map) const {
  json j;
  for (const auto& [key, value] : map) {
    if (auto p = std::any_cast<int>(&value))                            j[key] = *p;
    else if (auto p = std::any_cast<float>(&value))                     j[key] = *p;
    else if (auto p = std::any_cast<bool>(&value))                      j[key] = *p;
    else if (auto p = std::any_cast<std::string>(&value))               j[key] = *p;
    else if (auto p = std::any_cast<String>(&value))                    j[key] = p->c_str();
    else if (auto p = std::any_cast<std::map<std::string, std::any>>(&value)) j[key] = asJSON(*p);
    // unknown type → silently skipped
  }
  return j;
}
