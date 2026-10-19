/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/**
 * @file main.c
 * @brief Application entry point (plain C).
 *
 * The diot_matter wrapper hides the C++ Matter implementation.
 * The application performs three setup steps:
 *   a) initialize the LED,
 *   b) initialize the button (managed callbacks),
 *   c) start Matter connectivity.
 */

#include <inttypes.h>

#include "esp_log.h"

#ifndef CONFIG_DIOT_BUTTON_LONG_PRESS_TIME_MS
#define CONFIG_DIOT_BUTTON_LONG_PRESS_TIME_MS 5000
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "diot_button.h"
#include "diot_led.h"
#include "diot_matter.h"

static const char *TAG = "main";

/* Holding the button for at least this duration requests a factory reset. */
#define LONG_PRESS_MS CONFIG_DIOT_BUTTON_LONG_PRESS_TIME_MS

/**
 * @brief Button-to-Matter integration callback.
 *
 * The button FSM dispatches events from the application loop:
 *   - short press -> toggle the Matter light's OnOff attribute
 *   - long press  -> request a factory reset after releasing the button
 *
 * diot_matter_toggle_light() acquires the Matter stack lock.
 */
static void on_button_press(uint32_t duration_ms, void *arg)
{
    if (duration_ms >= LONG_PRESS_MS) {
        ESP_LOGW(TAG, "Long press (%" PRIu32 " ms): factory reset", duration_ms);
        diot_matter_factory_reset();
    } else {
        ESP_LOGI(TAG, "Short press (%" PRIu32 " ms): toggle light", duration_ms);
        diot_matter_toggle_light();
    }
}

void app_main(void)
{
    /* a) Addressable RGB LED hardware. */
    diot_led_fsm_t *led = diot_led_fsm_new();

    /* b) BOOT button FSM over managed SINGLE_CLICK/LONG_PRESS_UP callbacks. */
    diot_button_fsm_t *button = diot_button_fsm_new();
    diot_button_fsm_register_callback(button, on_button_press, NULL);

    /* c) Matter node, endpoints, clusters, commissioning, and network connectivity. */
    diot_matter_init(led);

    ESP_LOGI(TAG, "DIOT Matter light ready");

    while (1) {
        diot_button_fsm_fire(button);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
