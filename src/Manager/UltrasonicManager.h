#ifndef ULTRASONIC_MANAGER_H
#define ULTRASONIC_MANAGER_H


class UltrasonicManager {
public:
  UltrasonicManager(int trigPin, int echoPin);
  void initialize();
  float getDistance();
  float getLastDistance() const;
private:
  int trigPin;
  int echoPin;
  float lastDistance;
};

#endif // ULTRASONIC_MANAGER_H
