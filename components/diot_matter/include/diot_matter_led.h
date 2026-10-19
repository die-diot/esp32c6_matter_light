#pragma once

#include <stdint.h>

#include "diot_led.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DIOT_MATTER_LED_COLOR_MODE_HS = 0,
    DIOT_MATTER_LED_COLOR_MODE_TEMPERATURE,
    DIOT_MATTER_LED_COLOR_MODE_XY,
} diot_matter_led_color_mode_t;

typedef struct {
    bool on_off;
    uint8_t brightness;
    uint16_t hue;
    uint8_t saturation;
    uint32_t temperature;
    uint16_t x;
    uint16_t y;
    diot_matter_led_color_mode_t color_mode;
} diot_matter_led_attributes_t;

typedef struct {
    diot_led_fsm_t *base_led;
    diot_matter_led_attributes_t attributes;
} diot_matter_led_t;

esp_err_t diot_matter_led_init(diot_matter_led_t *p_matter_led, diot_led_fsm_t *p_base_led);

esp_err_t diot_matter_led_set_on_off(diot_matter_led_t *p_matter_led, bool on_off);
esp_err_t diot_matter_led_set_brightness_from_matter(diot_matter_led_t *p_matter_led, uint8_t brightness_matter);
esp_err_t diot_matter_led_set_hue_from_matter(diot_matter_led_t *p_matter_led, uint8_t hue_matter);
esp_err_t diot_matter_led_set_saturation_from_matter(diot_matter_led_t *p_matter_led, uint8_t saturation_matter);
esp_err_t diot_matter_led_set_temperature_mireds(diot_matter_led_t *p_matter_led, uint16_t temperature_mireds);
esp_err_t diot_matter_led_set_x(diot_matter_led_t *p_matter_led, uint16_t x);
esp_err_t diot_matter_led_set_y(diot_matter_led_t *p_matter_led, uint16_t y);

#ifdef __cplusplus
}
#endif
