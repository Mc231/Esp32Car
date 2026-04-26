// camera_pins.h

#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// Default to AI-Thinker if no model is set via build flags (preserves existing
// behavior for `pio run -e esp32cam`). Other envs in platformio.ini define
// their own CAMERA_MODEL_* macro.
#if !defined(CAMERA_MODEL_AI_THINKER) && !defined(CAMERA_MODEL_WROVER_KIT)
#define CAMERA_MODEL_AI_THINKER
#endif

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4

#elif defined(CAMERA_MODEL_WROVER_KIT)
// Espressif WROVER_KIT pin map. Used by ESP32-WROVER-DEV boards with the
// separate OV2640 daughter board on FFC ribbon (generic Aliexpress kit).
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     21
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       19
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM        5
#define Y2_GPIO_NUM        4
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM      -1

#else
#error "Camera model not selected"
#endif

#endif // CAMERA_PINS_H