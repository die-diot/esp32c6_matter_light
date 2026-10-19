/**
 * @file diot_led.c
 * @brief Base RGB LED driver implemented as a small FSM over led_strip.
 */

#include "diot_led.h"

#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#define LED_STRIP_GPIO_PIN      GPIO_NUM_8
#define LED_STRIP_RMT_RES_HZ    (10 * 1000 * 1000)

static const char *TAG = "diot_led";

/* ---------------------------------------------------------------------------
 * FSM states (kept private to this module).
 * ------------------------------------------------------------------------- */
typedef enum {
    DIOT_LED_FSM_STATE_IDLE = 0,
} diot_led_fsm_state_t;

/* ---------------------------------------------------------------------------
 * FSM check functions.
 * ------------------------------------------------------------------------- */
static bool check_update(const fsm_t *const p_fsm)
{
    const diot_led_fsm_t *const p_led_fsm = (const diot_led_fsm_t *)p_fsm;
    return p_led_fsm->flags.update;
}

/* ---------------------------------------------------------------------------
 * FSM action functions.
 * ------------------------------------------------------------------------- */
static void do_update_led(fsm_t *const p_fsm)
{
    diot_led_fsm_t *const p_led_fsm = (diot_led_fsm_t *)p_fsm;
    p_led_fsm->flags.update = false;

    diot_led_rgb_t rgb = p_led_fsm->state.rgb;
    if (!diot_led_fsm_get_state(p_led_fsm)) {
        rgb.r = 0;
        rgb.g = 0;
        rgb.b = 0;
    }

    ESP_ERROR_CHECK(led_strip_set_pixel(p_led_fsm->led_handle, 0, rgb.r, rgb.g, rgb.b));
    ESP_ERROR_CHECK(led_strip_refresh(p_led_fsm->led_handle));
    ESP_LOGI(TAG, "RGB LED color set to R:%u G:%u B:%u", rgb.r, rgb.g, rgb.b);
}

/* ---------------------------------------------------------------------------
 * FSM transition table.
 * ------------------------------------------------------------------------- */
static fsm_trans_t diot_led_fsm_transitions[] = {
    {DIOT_LED_FSM_STATE_IDLE, check_update, DIOT_LED_FSM_STATE_IDLE, do_update_led},
    {-1, NULL, -1, NULL},
};

/* ---------------------------------------------------------------------------
 * Public API.
 * ------------------------------------------------------------------------- */
diot_led_fsm_t *diot_led_fsm_new(void)
{
    diot_led_fsm_t *const p_led_fsm = (diot_led_fsm_t *)malloc(sizeof(diot_led_fsm_t));
    if (p_led_fsm == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for diot_led_fsm_t");
        return NULL;
    }
    diot_led_fsm_init(p_led_fsm);
    return p_led_fsm;
}

void diot_led_fsm_destroy(diot_led_fsm_t *const p_led_fsm)
{
    if (p_led_fsm == NULL) {
        return;
    }
    if (p_led_fsm->led_handle != NULL) {
        esp_err_t err = led_strip_del(p_led_fsm->led_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete led_strip handle: %s", esp_err_to_name(err));
        }
        p_led_fsm->led_handle = NULL;
    }
    free(p_led_fsm);
}

void diot_led_fsm_init(diot_led_fsm_t *const p_led_fsm)
{
    if (p_led_fsm == NULL) {
        ESP_LOGE(TAG, "Invalid pointer to LED FSM");
        return;
    }

    fsm_init(&p_led_fsm->fsm, diot_led_fsm_transitions);

    p_led_fsm->state = (diot_led_state_t){
        .state = false,
        .rgb = {255, 255, 255},
    };
    p_led_fsm->flags = (diot_led_flags_t){
        .update = false,
    };

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = 0,
        },
    };

    led_strip_handle_t led_strip = NULL;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    p_led_fsm->led_handle = led_strip;

    ESP_LOGI(TAG, "Created RGB LED FSM");
}

bool diot_led_fsm_get_state(const diot_led_fsm_t *const p_led_fsm)
{
    return p_led_fsm->state.state;
}

diot_led_rgb_t diot_led_fsm_get_rgb(const diot_led_fsm_t *const p_led_fsm)
{
    return p_led_fsm->state.rgb;
}

void diot_led_fsm_set_state(diot_led_fsm_t *const p_led_fsm, bool state)
{
    p_led_fsm->state.state = state;
    p_led_fsm->flags.update = true;
}

void diot_led_fsm_set_rgb(diot_led_fsm_t *const p_led_fsm, diot_led_rgb_t rgb)
{
    p_led_fsm->state.rgb = rgb;
    if (p_led_fsm->state.state) {
        p_led_fsm->flags.update = true;
    }
}
