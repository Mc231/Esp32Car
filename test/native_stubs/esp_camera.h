// Native stub for esp_camera.h. Just enough surface for CommandDispatcher's
// handleCamera() to compile — the body uses sensor_t / framesize_t /
// esp_camera_sensor_get / s->set_framesize / s->pixformat.
#ifndef NATIVE_ESP_CAMERA_STUB_H
#define NATIVE_ESP_CAMERA_STUB_H

typedef int framesize_t;

enum {
  PIXFORMAT_JPEG = 0,
  PIXFORMAT_RGB565 = 1,
};

struct sensor_t {
  int pixformat;
  int (*set_framesize)(sensor_t*, framesize_t);
};

inline sensor_t* esp_camera_sensor_get() { return nullptr; }

#endif
