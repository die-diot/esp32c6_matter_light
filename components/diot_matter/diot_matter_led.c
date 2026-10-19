#include "diot_matter_led.h"

#include <math.h>

typedef struct {
    uint16_t hue;
    uint8_t saturation;
} hs_color_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_color_t;

/** Standard max values used by the app-level LED model. */
#define STANDARD_BRIGHTNESS 100
#define STANDARD_HUE 360
#define STANDARD_SATURATION 100
#define STANDARD_TEMPERATURE_FACTOR 1000000

/** Matter max values used on Color/Level clusters. */
#define MATTER_BRIGHTNESS 254
#define MATTER_HUE 254
#define MATTER_SATURATION 254

static const hs_color_t s_temp_table[] = {
    {4, 100},  {8, 100},  {11, 100}, {14, 100}, {16, 100}, {18, 100}, {20, 100}, {22, 100}, {24, 100}, {25, 100},
    {27, 100}, {28, 100}, {30, 100}, {31, 100}, {31, 95},  {30, 89},  {30, 85},  {29, 80},  {29, 76},  {29, 73},
    {29, 69},  {28, 66},  {28, 63},  {28, 60},  {28, 57},  {28, 54},  {28, 52},  {27, 49},  {27, 47},  {27, 45},
    {27, 43},  {27, 41},  {27, 39},  {27, 37},  {27, 35},  {27, 33},  {27, 31},  {27, 30},  {27, 28},  {27, 26},
    {27, 25},  {27, 23},  {27, 22},  {27, 21},  {27, 19},  {27, 18},  {27, 17},  {27, 15},  {28, 14},  {28, 13},
    {28, 12},  {29, 10},  {29, 9},   {30, 8},   {31, 7},   {32, 6},   {34, 5},   {36, 4},   {41, 3},   {49, 2},
    {0, 0},    {294, 2},  {265, 3},  {251, 4},  {242, 5},  {237, 6},  {233, 7},  {231, 8},  {229, 9},  {228, 10},
    {227, 11}, {226, 11}, {226, 12}, {225, 13}, {225, 13}, {224, 14}, {224, 14}, {224, 15}, {224, 15}, {223, 16},
    {223, 16}, {223, 17}, {223, 17}, {223, 17}, {222, 18}, {222, 18}, {222, 19}, {222, 19}, {222, 19}, {222, 19},
    {222, 20}, {222, 20}, {222, 20}, {222, 21}, {222, 21},
};

static uint32_t remap_to_range(uint32_t value, uint32_t from_max, uint32_t to_max)
{
    if (from_max == 0) {
        return 0;
    }
    return (value * to_max) / from_max;
}

static uint32_t remap_to_range_inverse(uint32_t value, uint32_t factor)
{
    if (value == 0) {
        return 0;
    }
    return factor / value;
}

static void hsv_to_rgb(hs_color_t hs, uint8_t brightness, rgb_color_t *rgb)
{
    uint16_t hue = hs.hue % 360;
    uint16_t hi = (hue / 60) % 6;
    uint16_t f = 100 * hue / 60 - 100 * hi;
    uint16_t p = brightness * (100 - hs.saturation) / 100;
    uint16_t q = brightness * (10000 - f * hs.saturation) / 10000;
    uint16_t t = brightness * (10000 - hs.saturation * (100 - f)) / 10000;

    switch (hi) {
    case 0: rgb->red = brightness; rgb->green = t; rgb->blue = p; break;
    case 1: rgb->red = q; rgb->green = brightness; rgb->blue = p; break;
    case 2: rgb->red = p; rgb->green = brightness; rgb->blue = t; break;
    case 3: rgb->red = p; rgb->green = q; rgb->blue = brightness; break;
    case 4: rgb->red = t; rgb->green = p; rgb->blue = brightness; break;
    case 5: rgb->red = brightness; rgb->green = p; rgb->blue = q; break;
    default: break;
    }

    rgb->red = (uint8_t)(rgb->red * 255 / 100);
    rgb->green = (uint8_t)(rgb->green * 255 / 100);
    rgb->blue = (uint8_t)(rgb->blue * 255 / 100);
}

static void temp_to_hs(uint32_t temperature, hs_color_t *hs)
{
    if (temperature < 600) {
        hs->hue = 0;
        hs->saturation = 100;
        return;
    }
    if (temperature > 10000) {
        hs->hue = 222;
        hs->saturation = (uint8_t)(21 + (temperature - 10000) * 41 / 990000);
        return;
    }

    hs->hue = s_temp_table[(temperature - 600) / 100].hue;
    hs->saturation = s_temp_table[(temperature - 600) / 100].saturation;
}

static void xy_to_rgb(uint16_t x, uint16_t y, uint8_t brightness, rgb_color_t *rgb)
{
    float cx = (float)x / 65536.0f;
    float cy = (float)y / 65536.0f;
    float cz = 1.0f - cx - cy;

    float big_y = (float)brightness / 255.0f;
    float big_x = 0.0f;
    float big_z = 0.0f;

    if (cy > 0.0f) {
        big_x = (big_y / cy) * cx;
        big_z = (big_y / cy) * cz;
    }

    float r = big_x * 3.240479f - big_y * 1.537150f - big_z * 0.498535f;
    float g = -big_x * 0.969256f + big_y * 1.875992f + big_z * 0.041556f;
    float b = big_x * 0.055648f - big_y * 0.204043f + big_z * 1.057311f;

    r = (r <= 0.0031308f) ? (12.92f * r) : ((1.0f + 0.055f) * powf(r, (1.0f / 2.4f)) - 0.055f);
    g = (g <= 0.0031308f) ? (12.92f * g) : ((1.0f + 0.055f) * powf(g, (1.0f / 2.4f)) - 0.055f);
    b = (b <= 0.0031308f) ? (12.92f * b) : ((1.0f + 0.055f) * powf(b, (1.0f / 2.4f)) - 0.055f);

    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f;
    if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f;
    if (b > 1.0f) b = 1.0f;

    rgb->red = (uint8_t)(r * 255.0f);
    rgb->green = (uint8_t)(g * 255.0f);
    rgb->blue = (uint8_t)(b * 255.0f);
}

static esp_err_t diot_matter_led_apply(diot_matter_led_t *p_matter_led)
{
    if (p_matter_led == NULL || p_matter_led->base_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    rgb_color_t rgb = {0, 0, 0};
    const diot_matter_led_attributes_t *attrs = &p_matter_led->attributes;

    if (attrs->color_mode == DIOT_MATTER_LED_COLOR_MODE_XY) {
        uint8_t xy_brightness = (uint8_t)(attrs->brightness * 255 / 100);
        xy_to_rgb(attrs->x, attrs->y, xy_brightness, &rgb);
    } else {
        hs_color_t hs = {
            .hue = attrs->hue,
            .saturation = attrs->saturation,
        };

        if (attrs->color_mode == DIOT_MATTER_LED_COLOR_MODE_TEMPERATURE) {
            temp_to_hs(attrs->temperature, &hs);
        }
        hsv_to_rgb(hs, attrs->brightness, &rgb);
    }

    diot_led_fsm_set_rgb(p_matter_led->base_led, (diot_led_rgb_t){.r = rgb.red, .g = rgb.green, .b = rgb.blue});
    diot_led_fsm_set_state(p_matter_led->base_led, attrs->on_off);
    diot_led_fsm_fire(p_matter_led->base_led);
    return ESP_OK;
}

esp_err_t diot_matter_led_init(diot_matter_led_t *p_matter_led, diot_led_fsm_t *p_base_led)
{
    if (p_matter_led == NULL || p_base_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    p_matter_led->base_led = p_base_led;
    p_matter_led->attributes = (diot_matter_led_attributes_t){
        .on_off = false,
        .brightness = 100,
        .hue = 0,
        .saturation = 0,
        .temperature = 4000,
        .x = 0,
        .y = 0,
        .color_mode = DIOT_MATTER_LED_COLOR_MODE_HS,
    };
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_on_off(diot_matter_led_t *p_matter_led, bool on_off)
{
    if (p_matter_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.on_off = on_off;
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_brightness_from_matter(diot_matter_led_t *p_matter_led, uint8_t brightness_matter)
{
    if (p_matter_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.brightness = (uint8_t)remap_to_range(brightness_matter, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_hue_from_matter(diot_matter_led_t *p_matter_led, uint8_t hue_matter)
{
    if (p_matter_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.color_mode = DIOT_MATTER_LED_COLOR_MODE_HS;
    p_matter_led->attributes.hue = (uint16_t)remap_to_range(hue_matter, MATTER_HUE, STANDARD_HUE);
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_saturation_from_matter(diot_matter_led_t *p_matter_led, uint8_t saturation_matter)
{
    if (p_matter_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.color_mode = DIOT_MATTER_LED_COLOR_MODE_HS;
    p_matter_led->attributes.saturation = (uint8_t)remap_to_range(saturation_matter, MATTER_SATURATION, STANDARD_SATURATION);
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_temperature_mireds(diot_matter_led_t *p_matter_led, uint16_t temperature_mireds)
{
    if (p_matter_led == NULL || temperature_mireds == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.color_mode = DIOT_MATTER_LED_COLOR_MODE_TEMPERATURE;
    p_matter_led->attributes.temperature = remap_to_range_inverse(temperature_mireds, STANDARD_TEMPERATURE_FACTOR);
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_x(diot_matter_led_t *p_matter_led, uint16_t x)
{
    if (p_matter_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.color_mode = DIOT_MATTER_LED_COLOR_MODE_XY;
    p_matter_led->attributes.x = x;
    return diot_matter_led_apply(p_matter_led);
}

esp_err_t diot_matter_led_set_y(diot_matter_led_t *p_matter_led, uint16_t y)
{
    if (p_matter_led == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    p_matter_led->attributes.color_mode = DIOT_MATTER_LED_COLOR_MODE_XY;
    p_matter_led->attributes.y = y;
    return diot_matter_led_apply(p_matter_led);
}
