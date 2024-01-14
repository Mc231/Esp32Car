
struct CarApplicationConfig {
    int leftMotorPin1;
    int leftMotorPin2;
    int leftMotorPwm;
    int rightMotorPin1;
    int rightMotorPin2;
    int rightMotorPwm;
    int ultrasonicPin1;
    int ultrasonicPin2;

     CarApplicationConfig() 
        : leftMotorPin1(2),    // Initialize with default values
          leftMotorPin2(14),
          leftMotorPwm(4),
          rightMotorPin1(15),
          rightMotorPin2(13),
          rightMotorPwm(12),
          ultrasonicPin1(3),
          ultrasonicPin2(1) {}
};