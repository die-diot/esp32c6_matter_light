/**
 * @file diot_button.c
 * @brief Button FSM wrapper built on top of managed espressif/button callbacks.
 */

#include "diot_button.h"

#include <inttypes.h>
#include <stdlib.h>

#include "button_gpio.h"
#include "esp_log.h"

#ifndef CONFIG_DIOT_BUTTON_LONG_PRESS_TIME_MS
#define CONFIG_DIOT_BUTTON_LONG_PRESS_TIME_MS 5000
#endif

static const char *TAG = "diot_button";
static const gpio_num_t BUTTON_GPIO_PIN = GPIO_NUM_9;
static const uint32_t BUTTON_DEBOUNCE_MS = 150;
static const uint32_t BUTTON_LONG_PRESS_MS = CONFIG_DIOT_BUTTON_LONG_PRESS_TIME_MS;

/* ---------------------------------------------------------------------------
 * FSM states (kept private to this module).
 * ------------------------------------------------------------------------- */
typedef enum {
    DIOT_BUTTON_FSM_STATE_IDLE = 0,
} diot_button_fsm_state_t;

/* ---------------------------------------------------------------------------
 * Managed button callbacks.
 * ------------------------------------------------------------------------- */
static void _on_single_click_cb(void *button_handle, void *user_data)
{
    diot_button_fsm_t *const p_fsm = (diot_button_fsm_t *)user_data;
    uint32_t duration_ms = iot_button_get_pressed_time((button_handle_t)button_handle);
    p_fsm->pending_duration = duration_ms;
    p_fsm->pending_event = true;
}

static void _on_long_press_up_cb(void *button_handle, void *user_data)
{
    diot_button_fsm_t *const p_fsm = (diot_button_fsm_t *)user_data;
    uint32_t duration_ms = iot_button_get_pressed_time((button_handle_t)button_handle);
    p_fsm->pending_duration = duration_ms;
    p_fsm->pending_event = true;
}

/* ---------------------------------------------------------------------------
 * FSM check functions.
 * ------------------------------------------------------------------------- */
static bool check_pending_event(const fsm_t *const p_this)
{
    const diot_button_fsm_t *const p_fsm = (const diot_button_fsm_t *)p_this;
    return p_fsm->pending_event;
}

/* ---------------------------------------------------------------------------
 * FSM action functions.
 * ------------------------------------------------------------------------- */
static void do_dispatch_event(fsm_t *const p_this)
{
    diot_button_fsm_t *const p_fsm = (diot_button_fsm_t *)p_this;
    if (!p_fsm->pending_event) {
        return;
    }

    p_fsm->duration = p_fsm->pending_duration;
    p_fsm->pending_duration = 0;
    p_fsm->pending_event = false;

    ESP_LOGI(TAG, "Button event on GPIO%d: %" PRIu32 " ms", (int)p_fsm->gpio, p_fsm->duration);
    if (p_fsm->callback != NULL) {
        p_fsm->callback(p_fsm->duration, p_fsm->callback_arg);
    }
}

/* ---------------------------------------------------------------------------
 * FSM transition table.
 * ------------------------------------------------------------------------- */
static fsm_trans_t diot_button_fsm_transitions[] = {
    {DIOT_BUTTON_FSM_STATE_IDLE, check_pending_event, DIOT_BUTTON_FSM_STATE_IDLE, do_dispatch_event},
    {-1, NULL, -1, NULL},
};

/* ---------------------------------------------------------------------------
 * Public API.
 * ------------------------------------------------------------------------- */
diot_button_fsm_t *diot_button_fsm_new(void)
{
    diot_button_fsm_t *const p_fsm = (diot_button_fsm_t *)malloc(sizeof(diot_button_fsm_t));
    if (p_fsm == NULL) {
        ESP_LOGE(TAG, "Failed to allocate button wrapper");
        return NULL;
    }

    diot_button_fsm_init(p_fsm);
    return p_fsm;
}

void diot_button_fsm_destroy(diot_button_fsm_t *const p_button_fsm)
{
    if (p_button_fsm == NULL) {
        return;
    }
    if (p_button_fsm->handle != NULL) {
        iot_button_delete(p_button_fsm->handle);
        p_button_fsm->handle = NULL;
    }
    free(p_button_fsm);
}

void diot_button_fsm_register_callback(diot_button_fsm_t *const p_button_fsm, diot_button_callback_t callback, void *arg)
{
    if (p_button_fsm == NULL) {
        return;
    }
    p_button_fsm->callback = callback;
    p_button_fsm->callback_arg = arg;
}

void diot_button_fsm_init(diot_button_fsm_t *const p_fsm)
{
    if (p_fsm == NULL) {
        ESP_LOGE(TAG, "Invalid pointer to button FSM");
        return;
    }

    fsm_init(&p_fsm->fsm, diot_button_fsm_transitions);

    p_fsm->gpio = BUTTON_GPIO_PIN;
    p_fsm->active_low = true;
    p_fsm->debounce_time_ms = BUTTON_DEBOUNCE_MS;
    p_fsm->long_press_time_ms = BUTTON_LONG_PRESS_MS;
    p_fsm->pending_event = false;
    p_fsm->pending_duration = 0;
    p_fsm->duration = 0;
    p_fsm->handle = NULL;
    p_fsm->callback = NULL;
    p_fsm->callback_arg = NULL;

    // Create the managed button device
    // Note: the iot_button library expects the long press time to be greater than the short press time (debounce)
    button_config_t config = {
        .long_press_time = (uint16_t)p_fsm->long_press_time_ms,
        .short_press_time = (uint16_t)p_fsm->debounce_time_ms,
    };

    // Configure the GPIO button device
    button_gpio_config_t gpio_config = {
        .gpio_num = (int32_t)p_fsm->gpio,
        .active_level = BUTTON_INACTIVE,
        .enable_power_save = false,
        .disable_pull = false,
    };

    // Create the button device and register callbacks
    esp_err_t err = iot_button_new_gpio_device(&config, &gpio_config, &p_fsm->handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "iot_button_new_gpio_device failed: %s", esp_err_to_name(err));
        return;
    }

    // Register the single click and long press callbacks
    err = iot_button_register_cb(p_fsm->handle, BUTTON_SINGLE_CLICK, NULL, _on_single_click_cb, p_fsm);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register BUTTON_SINGLE_CLICK callback: %s", esp_err_to_name(err));
        iot_button_delete(p_fsm->handle);
        p_fsm->handle = NULL;
        return;
    }

    err = iot_button_register_cb(p_fsm->handle, BUTTON_LONG_PRESS_UP, NULL, _on_long_press_up_cb, p_fsm);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register BUTTON_LONG_PRESS_UP callback: %s", esp_err_to_name(err));
        iot_button_delete(p_fsm->handle);
        p_fsm->handle = NULL;
        return;
    }

    ESP_LOGI(TAG, "Managed button ready on GPIO%d (debounce %" PRIu32 " ms, long %" PRIu32 " ms)",
             (int)p_fsm->gpio, p_fsm->debounce_time_ms, p_fsm->long_press_time_ms);
}