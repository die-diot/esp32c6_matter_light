#ifndef DIOT_MATTER_H
#define DIOT_MATTER_H

/**
 * @file diot_matter.h
 * @brief Matter stack wrapper with a plain C API.
 *
 * Hides C++ stack initialization, commissioning, and network startup.
 * The diot_matter_devices module builds endpoints and clusters and routes
 * attribute callbacks to the C LED adapter.
 */

#include "diot_led.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the Matter stack.
 *
 * Initialize NVS, create an Extended Color Light endpoint and its clusters,
 * configure commissioning, and start connectivity. The endpoint retains
 * the LED adapter as private data so attribute updates reach the LED.
 *
 * @param led_handle LED instance created with diot_led_fsm_new().
 * @return ESP_OK on success, or an error code.
 */
esp_err_t diot_matter_init(diot_led_fsm_t *led_handle);

/**
 * @brief Toggle the light's OnOff attribute while holding the stack lock.
 *
 * Acquire ScopedChipStackLock before reading and updating the attribute.
 * This is the integration point for a button event in the application task.
 */
void diot_matter_toggle_light(void);

/**
 * @brief Request a Matter factory reset, for example after a long press.
 */
void diot_matter_factory_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DIOT_MATTER_H */
