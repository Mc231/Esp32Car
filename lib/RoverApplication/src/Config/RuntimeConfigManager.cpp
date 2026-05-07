#include "RuntimeConfigManager.h"
#include "Log/RemoteLogger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

RuntimeConfigManager::RuntimeConfigManager(AbstractFS& fs) : fileSystem(fs) {}

void RuntimeConfigManager::ensureMounted() {
  if (!fsMounted) {
    fsMounted = fileSystem.begin();
    if (!fsMounted) Log.println("RuntimeConfigManager: filesystem mount failed");
  }
}

const RuntimeConfigManager::Config& RuntimeConfigManager::read() {
  if (cachedLoaded) return cached;
  ensureMounted();
  cachedLoaded = true;
  if (!fsMounted || !fileSystem.exists(CONFIG_FILE)) return cached;

  File f = fileSystem.open(CONFIG_FILE, "r");
  if (!f) return cached;
  String body;
  while (f.available()) body += (char)f.read();
  f.close();

  json j = json::parse(body.c_str(), nullptr, false);
  if (j.is_discarded()) {
    Log.println("RuntimeConfigManager: config file is not valid JSON, ignoring");
    return cached;
  }

  if (j.contains("mqtt")) {
    auto& m = j["mqtt"];
    if (m.contains("enabled"))     cached.mqttEnabled = m["enabled"].get<bool>();
    if (m.contains("host"))        cached.mqttHost = String(m["host"].get<std::string>().c_str());
    if (m.contains("port"))        cached.mqttPort = m["port"].get<int>();
    if (m.contains("user"))        cached.mqttUser = String(m["user"].get<std::string>().c_str());
    if (m.contains("password"))    cached.mqttPassword = String(m["password"].get<std::string>().c_str());
    if (m.contains("clientId"))    cached.mqttClientId = String(m["clientId"].get<std::string>().c_str());
    if (m.contains("topicPrefix")) cached.mqttTopicPrefix = String(m["topicPrefix"].get<std::string>().c_str());
  }
  return cached;
}

void RuntimeConfigManager::save(const Config& cfg) {
  ensureMounted();
  if (!fsMounted) return;
  json j;
  j["mqtt"]["enabled"]     = cfg.mqttEnabled;
  j["mqtt"]["host"]        = std::string(cfg.mqttHost.c_str());
  j["mqtt"]["port"]        = cfg.mqttPort;
  j["mqtt"]["user"]        = std::string(cfg.mqttUser.c_str());
  j["mqtt"]["password"]    = std::string(cfg.mqttPassword.c_str());
  j["mqtt"]["clientId"]    = std::string(cfg.mqttClientId.c_str());
  j["mqtt"]["topicPrefix"] = std::string(cfg.mqttTopicPrefix.c_str());
  std::string body = j.dump();

  File f = fileSystem.open(CONFIG_FILE, "w");
  if (!f) {
    Log.println("RuntimeConfigManager: open for write failed");
    return;
  }
  f.print(body.c_str());
  f.close();
  cached = cfg;
  cachedLoaded = true;
}

void RuntimeConfigManager::clear() {
  ensureMounted();
  if (!fsMounted) return;
  if (fileSystem.exists(CONFIG_FILE)) fileSystem.remove(CONFIG_FILE);
  cached = Config{};
  cachedLoaded = true;
}
