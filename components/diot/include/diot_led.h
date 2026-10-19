#ifndef DIOT_LED_H
#define DIOT_LED_H

#include <stdbool.h>
#include <stdint.h>

#include "fsm.h"
#include "led_strip.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RGB color used by the base LED driver.
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} diot_led_rgb_t;

/**
 * @brief Public LED state snapshot (power + RGB).
 */
typedef struct {
    bool state;
    diot_led_rgb_t rgb;
} diot_led_state_t;

/**
 * @brief Internal flags consumed by the LED FSM.
 */
typedef struct {
    volatile bool update;
} diot_led_flags_t;

/**
 * @brief Base LED FSM (protocol-agnostic).
 *
 * This struct intentionally knows nothing about Matter/MQTT.
 */
typedef struct {
    fsm_t fsm;
    diot_led_state_t state;
    diot_led_flags_t flags;
    led_strip_handle_t led_handle;
} diot_led_fsm_t;

/**
 * @brief Allocate and initialize a LED FSM instance.
 *
 * @return Pointer to a ready-to-use LED FSM, or NULL on allocation failure.
 */
diot_led_fsm_t *diot_led_fsm_new(void);

/**
 * @brief Release resources associated with a LED FSM instance.
 */
void diot_led_fsm_destroy(diot_led_fsm_t *const p_led_fsm);

/**
 * @brief Initialize an already allocated LED FSM instance.
 */
void diot_led_fsm_init(diot_led_fsm_t *const p_led_fsm);

/**
 * @brief Fire the LED FSM once.
 */
static inline void diot_led_fsm_fire(const diot_led_fsm_t * p_led_fsm)
{
    fsm_fire((fsm_t *)p_led_fsm);
}

/**
 * @brief Read current on/off state from the LED FSM.
 */
bool diot_led_fsm_get_state(const diot_led_fsm_t * const p_led_fsm);

/**
 * @brief Read current RGB state from the LED FSM.
 */
diot_led_rgb_t diot_led_fsm_get_rgb(const diot_led_fsm_t * const p_led_fsm);

/**
 * @brief Set target on/off state and mark hardware update pending.
 */
void diot_led_fsm_set_state(diot_led_fsm_t * const p_led_fsm, bool state);

/**
 * @brief Set target RGB and mark hardware update pending when LED is on.
 */
void diot_led_fsm_set_rgb(diot_led_fsm_t * const p_led_fsm, diot_led_rgb_t rgb);

#ifdef __cplusplus
}
#endif

#endif
