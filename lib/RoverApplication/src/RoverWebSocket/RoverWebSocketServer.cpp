#include "RoverWebSocketServer.h"

RoverWebSocketServer::RoverWebSocketServer(RoverController& carController, RoverApplicationConfig& config, SystemMonitor& systemMonitor)
  : carController(carController), config(config), systemMonitor(systemMonitor), wsServer(config.webSocketPort)  {}


void RoverWebSocketServer::begin() {
    wsServer.begin();
    wsServer.onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
        this->handleWebSocketMessage(num, type, payload, length);
    });
}

void RoverWebSocketServer::loop() {
    wsServer.loop();
}

void RoverWebSocketServer::handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        payload[length] = 0; // Ensure the data is null-terminated
        String message = (char *)payload;
        // Parse the incoming message as JSON
        json j = json::parse(message.c_str(), nullptr, false);
        if (j.is_discarded()) {
            Serial.println("Received message is not valid JSON");
            return;
        }

        // Check for the "command" key in the JSON object
        if (j.contains("command")) {
            std::string command = j["command"].get<std::string>();
            Serial.print("Received command: ");
            Serial.println(command.c_str());

            if (command == "system")
            {
                handleSystem();
                return;
            } else if (command == "status")
            {
                handleStatus();
                return;
            } else if (command == "config")
            {
                handleConfig();
                return;
            } else if (command == "reboot") {
                handleReboot(); 
                return;
            } else if(command == "wifi") {
                handleGetWiFi();
                return;
            } else if(command == "forget_wi_fi") {
                handleWiFiForget();
                return;
            } else if (command == "ultrasonic") 
            {
                handleGetUltrasonic();
                return;
            } else if (command == "set_camera") {
                handleCamera(j);
                return;
            } else if (command == "motor_state")
            {
                handleMotorState();
                return;
            } else if (command == "set_motor") {
                handleSetMotor(j);
                return;
            } else if (command == "set_motor_pwm")
            {
               handleSetMotorPWM(j);
               return;
            }
        } else {
            Serial.println("No command specified in message");
        }
    }
}

void RoverWebSocketServer::handleSystem() {
  std::map<std::string, std::any> dataMap = {{"response", systemMonitor.getState()}};
  sendData(dataMap);
}

void RoverWebSocketServer::handleStatus() {
    std::map<std::string, std::any> result;
    result["motor"] = carController.getMotorState();
    result["system"] = systemMonitor.getState();
    std::map<std::string, std::any> dataMap = {{"response", result}};
    sendData(dataMap);
}

void RoverWebSocketServer::handleConfig() {
    std::map<std::string, std::any> result;
    result["leftMotorPin1"] = config.leftMotorPin1;
    result["leftMotorPin2"] = config.leftMotorPin2;
    result["leftMotorPwm"] = config.leftMotorPwm;
    result["rightMotorPin1"] = config.rightMotorPin1;
    result["rightMotorPin2"] = config.rightMotorPin2;
    result["rightMotorPwm"] = config.rightMotorPwm;
    result["ultrasonicPin1"] = config.ultrasonicPin1;
    result["ultrasonicPin2"] = config.ultrasonicPin2;
    result["ultrasonic_enabled"] = config.ultrasonicSensorEnabled;
    std::map<std::string, std::any> dataMap = {{"response", result}};
    sendData(dataMap);
}

void RoverWebSocketServer::handleReboot() {
    std::map<std::string, std::any> dataMap = {{"status", std::string("rebooting")}};
    sendData( dataMap);
    carController.reboot();
}

void RoverWebSocketServer::handleGetWiFi() {
    auto response = carController.getWiFiConfig();
    std::map<std::string, std::any> dataMap = {{"response", response}};
    sendData(dataMap);
}

void RoverWebSocketServer::handleWiFiForget() {
    carController.forgotWiFi();
    std::map<std::string, std::any> dataMap = {{"status", std::string("Forget WI-FI Success")}};
    sendData(dataMap);
    carController.reboot();
}

void RoverWebSocketServer::handleGetUltrasonic() {
    std::map<std::string, std::any> dataMap = {{"response", carController.getUltrasonicState()}};
    sendData(dataMap);
}

// TODO: - Require updates
void RoverWebSocketServer::handleCamera(const json& j) {
    if (j.contains("frame_size")) {
        int size = j["frame_size"].get<int>();
        sensor_t *s = esp_camera_sensor_get();
        s->set_framesize(s, (framesize_t)size);
        wsServer.broadcastTXT("{\"status\": \"Camera frame size updated\"}");
    } else {
        std::map<std::string, std::any> result;
        result["status"] = "Frame size parameter missing";
        sendData(result);
    }
}

void RoverWebSocketServer::handleSetMotorPWM(const nlohmann::json& j) {
    if (j.contains("pwm") && j.contains("motor")) {
        int motor = j["motor"].get<int>();
        MotorSelection selection = static_cast<MotorSelection>(motor);
        int pwmValue = j["pwm"].get<int>(); 
        carController.setMotorSpeed(selection, pwmValue);
        this->handleMotorState();
    } else {
        wsServer.broadcastTXT("{\"status\": \"pwm or motor is not passed\"}");
    }
}

void RoverWebSocketServer::handleMotorState() {
  std::map<std::string, std::any> dataMap = {{"response", carController.getMotorState()}};
  sendData(dataMap);
}

void RoverWebSocketServer::handleSetMotor(const nlohmann::json& j) {
    if (j.contains("action") && j.contains("motor")) {
        MotorAction action = static_cast<MotorAction>(j["action"].get<int>());
        MotorSelection selection = static_cast<MotorSelection>(j["motor"].get<int>());
        carController.setMotorAction(action, selection);
        this->handleMotorState();
    } else {
         wsServer.broadcastTXT("{\"status\": \"Action or motor is not passed\"}");
    }
}

void RoverWebSocketServer::sendData(const std::map<std::string, std::any>& dataMap) {
    json j = asJSON(dataMap);
    String message = j.dump().c_str();
    wsServer.broadcastTXT(message);
}

json RoverWebSocketServer::asJSON(const std::map<std::string, std::any>& map) const {
    json j;
    for (const auto& item : map) {
        const auto& key = item.first;
        const auto& value = item.second;

        try {
            j[key] = std::any_cast<String>(value).c_str();
        } catch (const std::bad_any_cast&) {
            try {
                j[key] = std::any_cast<std::string>(value);
            } catch (const std::bad_any_cast&) {
                try {
                    j[key] = std::any_cast<int>(value);
                } catch (const std::bad_any_cast&) {
                    try {
                        j[key] = std::any_cast<float>(value);
                    } catch (const std::bad_any_cast&) {
                        try {
                            // This handles a nested std::map<std::string, std::any>
                            j[key] = asJSON(std::any_cast<std::map<std::string, std::any>>(value));
                        } catch (const std::bad_any_cast&) {
                            // Handle other types or ignore.
                        }
                    }
                }
            }
        }
    }
    return j;
}
