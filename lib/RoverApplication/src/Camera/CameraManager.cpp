#include "CameraManager.h"
#include "Arduino.h"
#include "Log/RemoteLogger.h"


CameraManager::CameraManager() { }

CameraManager::~CameraManager() { }

void CameraManager::initialize() {
    setupCamera();
}


void CameraManager::setupCamera()  {
#ifdef ROVER_BOARD_WROVER_CAM
  // OV2640 daughter board on the WROVER-DEV needs ~1-2s after power-on
  // before SCCB will respond. AI-Thinker doesn't need this (camera shares
  // ESP32 VCC).
  delay(2000);
#endif

  // Zero-init: esp_camera reads fields like sccb_i2c_port that this code
  // doesn't set explicitly. Uninitialized stack garbage there causes the
  // SCCB probe to fail with 0x105 on some boards.
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  // Streaming framesize. VGA = 640x480 — good quality, fits comfortably in
  // PSRAM. Drop to QVGA (320x240) if Wi-Fi struggles or capture starts failing.
  static constexpr framesize_t kStreamFrameSize = FRAMESIZE_VGA;

  config.xclk_freq_hz = 20000000;
  config.frame_size = kStreamFrameSize;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = psramFound() ? 2 : 1;
  if (!psramFound()) {
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Log.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

#ifdef ROVER_BOARD_WROVER_CAM
  // OV2640 on the WROVER-DEV daughter board is mounted upside-down relative
  // to the rover chassis. Flip vertically (and horizontally — otherwise the
  // image would be mirrored after the vflip).
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  // Re-apply framesize to ensure sensor is actively streaming. Same size as
  // init so frame buffers stay valid.
  s->set_framesize(s, kStreamFrameSize);
}
