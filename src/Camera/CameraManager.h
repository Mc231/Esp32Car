#ifndef CameraManager_h
#define CameraManager_h

#include "camera_pins.h"
#include "esp_camera.h"


class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    void initialize();

private:
    void setupCamera();
};

#endif // CameraManager_h
