#include "BLECallbacks.h"

// Constructor initialization

SetServoCallback::SetServoCallback(CarController *controller) : carController(controller) {}

GetServoCallback::GetServoCallback(CarController *controller) : carController(controller) {}

SetMotorCallback::SetMotorCallback(CarController *controller) : carController(controller) {}

GetMotorCallback::GetMotorCallback(CarController *controller) : carController(controller) {}

GetUltrasonicCallback::GetUltrasonicCallback(CarController *controller) : carController(controller) {}

// SetServoCallback methods

void SetServoCallback::onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() == 2) {  // assuming 1 byte for position and 1 byte for servo number
        uint8_t pos = value[0];
        uint8_t servo = value[1];
        
        carController->setServoPosition(static_cast<int>(pos), static_cast<ServoSelection>(servo));
    }
}

// GetServoCallback methods

void GetServoCallback::onRead(BLECharacteristic *pCharacteristic) {
    auto servoStates = carController->getServoState();
    // Assuming the servo states map contains two key-value pairs: {"servo1": position1, "servo2": position2}
    int servo1Pos = std::any_cast<int>(servoStates["servo1"]);
    int servo2Pos = std::any_cast<int>(servoStates["servo2"]);

    std::string response = std::to_string(servo1Pos) + "," + std::to_string(servo2Pos);  // example format
    pCharacteristic->setValue(response);
}

// SetMotorCallback methods

void SetMotorCallback::onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() == 2) {  // assuming 1 byte for action and 1 byte for motor number
        uint8_t action = value[0];
        uint8_t motor = value[1];
        
        carController->setMotorAction(static_cast<MotorAction>(action), static_cast<MotorSelection>(motor));
    }
}

void GetMotorCallback::onRead(BLECharacteristic *pCharacteristic) {
    auto motorStates = carController->getMotorState();

    // Here, I'm making the assumption based on your previous code snippets that the motor states map contains key-value pairs for each motor's state.
    // This is a simple example. Depending on the actual contents of the `motorStates` map, you might need to adjust this.
    int leftMotorState = std::any_cast<int>(motorStates["leftMotor"]);
    int rightMotorState = std::any_cast<int>(motorStates["rightMotor"]);

    std::string response = "Left Motor: " + std::to_string(leftMotorState) + ", Right Motor: " + std::to_string(rightMotorState);  // example format
    pCharacteristic->setValue(response);
}

SetMotorPWMCallback::SetMotorPWMCallback(CarController *controller) : carController(controller) {}

void SetMotorPWMCallback::onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
        // For this example, I'm assuming that the incoming format is: "motorSelection,pwmValue"
        // E.g., "left,128" or "both,255"
        size_t delimiterPos = value.find(',');
        std::string motor = value.substr(0, delimiterPos);
        int pwmValue = std::stoi(value.substr(delimiterPos + 1));

        MotorSelection motorSelection;
        if (motor == "left") {
            motorSelection = MotorSelection::LEFT;
        } else if (motor == "right") {
            motorSelection = MotorSelection::RIGHT;
        } else if (motor == "both") {
            motorSelection = MotorSelection::ALL;
        }

        carController->setMotorSpeed(motorSelection, pwmValue);
    }
}

// GetUltrasonicCallback methods

void GetUltrasonicCallback::onRead(BLECharacteristic *pCharacteristic) {
    float distance = carController->getLastUltrasonicDistance();
    pCharacteristic->setValue(std::to_string(distance));
}
