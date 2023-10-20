#ifndef BLECallbacks_h
#define BLECallbacks_h

#include <BLEDevice.h>
#include "CarController.h"

class SetServoCallback : public BLECharacteristicCallbacks {
    CarController *carController;
public:
    SetServoCallback(CarController *controller);
    void onWrite(BLECharacteristic *pCharacteristic);
};

class GetServoCallback : public BLECharacteristicCallbacks {
    CarController *carController;
public:
    GetServoCallback(CarController *controller);
    void onRead(BLECharacteristic *pCharacteristic);
};

class GetMotorCallback : public BLECharacteristicCallbacks {
public:
    explicit GetMotorCallback(CarController *controller);
    void onRead(BLECharacteristic *pCharacteristic) override;

private:
    CarController *carController;
};

class SetMotorCallback : public BLECharacteristicCallbacks {
    CarController *carController;
public:
    SetMotorCallback(CarController *controller);
    void onWrite(BLECharacteristic *pCharacteristic);
};

class SetMotorPWMCallback : public BLECharacteristicCallbacks {
public:
    explicit SetMotorPWMCallback(CarController *controller);
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    CarController *carController;
};


class GetUltrasonicCallback : public BLECharacteristicCallbacks {
    CarController *carController;
public:
    GetUltrasonicCallback(CarController *controller);
    void onRead(BLECharacteristic *pCharacteristic);
};

#endif
