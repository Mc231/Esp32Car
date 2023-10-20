#include "BLEManager.h"
#include "BLECallbacks.h"



BLEManager::BLEManager(CarController& carController) 
    : carController(carController)
{}

void BLEManager::begin() {

  BLEDevice::init("EvaCar");
  BLEServer* pServer = BLEDevice::createServer();

   BLESecurity* pSecurity = new BLESecurity();
   pSecurity->setStaticPIN(123456);  

    // Creating Servo Service
    BLEService* pServoService = pServer->createService(SERVO_SERVICE_UUID);
    BLECharacteristic *pSetServoChar = pServoService->createCharacteristic(
        SET_SERVO_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pSetServoChar->setCallbacks(new SetServoCallback(&carController));
    pServoService->start();

    // Creating Motor Service
    BLEService* pMotorService = pServer->createService(MOTOR_SERVICE_UUID);
    BLECharacteristic *pSetMotorChar = pMotorService->createCharacteristic(
        SET_MOTOR_ACTION_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pSetMotorChar->setCallbacks(new SetMotorCallback(&carController));
    
    BLECharacteristic *pGetMotorStateChar = pMotorService->createCharacteristic(
        GET_MOTOR_STATE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pGetMotorStateChar->setCallbacks(new GetMotorCallback(&carController));

    BLECharacteristic *pSetMotorPWMChar = pMotorService->createCharacteristic(
        SET_MOTOR_PWM_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pSetMotorPWMChar->setCallbacks(new SetMotorPWMCallback(&carController));
    pMotorService->start();

    // Creating Ultrasonic Service
    BLEService* pUltrasonicService = pServer->createService(ULTRASONIC_SERVICE_UUID);
    BLECharacteristic *pGetUltrasonicChar = pUltrasonicService->createCharacteristic(
        GET_ULTRASONIC_DISTANCE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pGetUltrasonicChar->setCallbacks(new GetUltrasonicCallback(&carController));
    pUltrasonicService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVO_SERVICE_UUID);
    pAdvertising->addServiceUUID(MOTOR_SERVICE_UUID);
    pAdvertising->addServiceUUID("12345678-abcd-1234-abcd-1234567890cb");
    pAdvertising->start();
}
