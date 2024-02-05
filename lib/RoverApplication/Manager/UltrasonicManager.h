#ifndef ULTRASONIC_MANAGER_H
#define ULTRASONIC_MANAGER_H

#include <map>
#include <any>

class UltrasonicManager {
public:
  UltrasonicManager(int trigPin, int echoPin);
  void initialize();
  float getDistance();
  float getLastDistance() const;
  std::map<std::string, std::any> getState();
private:
  int trigPin;
  int echoPin;
  float lastDistance;
};

#endif // ULTRASONIC_MANAGER_H
