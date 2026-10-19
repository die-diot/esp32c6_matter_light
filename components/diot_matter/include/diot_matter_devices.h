#pragma once

#include "diot_led.h"
#include "esp_err.h"
#include "esp_matter.h"

namespace diot::matter::devices {

esp_err_t create_light_device(esp_matter::node_t *node, diot_led_fsm_t *led_handle, uint16_t *light_endpoint_id);
esp_err_t apply_light_defaults(uint16_t light_endpoint_id);

esp_err_t identification_cb(esp_matter::identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                            uint8_t effect_variant, void *priv_data);

esp_err_t attribute_update_cb(esp_matter::attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                              uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);

} // namespace diot::matter::devices
