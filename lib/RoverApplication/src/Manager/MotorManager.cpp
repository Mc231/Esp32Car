#include "MotorManager.h"

// Motor PWM frequency and resolution. 5kHz at 8 bits is the Arduino-ESP32
// default for analogWrite() and works fine for L298N inputs.
static constexpr uint32_t MOTOR_PWM_FREQ = 5000;
static constexpr uint8_t  MOTOR_PWM_BITS = 8;

MotorManager::MotorManager(int pinA, int pinB, int pwm, int ledcChannel) {
  this->pinA = pinA;
  this->pinB = pinB;
  this->pwm = pwm;
  this->ledcChannel = ledcChannel;
  currentAction = STOP;
  currentSpeed = 0;
  // Hardware init deferred to begin(): pinMode + ledc setup at static-init
  // run before Arduino/LEDC is ready and silently fail to attach the channel.
}

void MotorManager::begin() {
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  // Bind this pin to a SPECIFIC LEDC channel. We deliberately avoid
  // analogWrite(), which auto-allocates from channel 0 upward and would
  // clobber esp_camera's XCLK (channel 0, 20MHz on the camera XCLK pin).
  ledcSetup(ledcChannel, MOTOR_PWM_FREQ, MOTOR_PWM_BITS);
  ledcAttachPin(pwm, ledcChannel);
  ledcWrite(ledcChannel, 0);
  action(STOP);
}

void MotorManager::action(MotorAction act) {
  currentAction = act;
  switch (act) {
    case FORWARD:
      digitalWrite(pinA, HIGH);
      digitalWrite(pinB, LOW);
      ledcWrite(ledcChannel, currentSpeed);
      break;
    case BACKWARD:
      digitalWrite(pinA, LOW);
      digitalWrite(pinB, HIGH);
      ledcWrite(ledcChannel, currentSpeed);
      break;
    case STOP:
      digitalWrite(pinA, LOW);
      digitalWrite(pinB, LOW);
      ledcWrite(ledcChannel, currentSpeed);
      break;
  }
}

void MotorManager::changeSpeed(int speed) {
  currentSpeed = constrain(speed, 0, 255);
  if(currentAction == FORWARD || currentAction == BACKWARD) {
    ledcWrite(ledcChannel, currentSpeed);
  }
}

int MotorManager::getCurrentSpeed() {
  return currentSpeed;
}

MotorAction MotorManager::getCurrentAction() {
  return currentAction;
}
