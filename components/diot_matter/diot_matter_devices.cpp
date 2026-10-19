#include "diot_matter_devices.h"

#include <esp_log.h>
#include <app/data-model/Decode.h>

#include "diot_matter_led.h"

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

namespace {

static const char *TAG = "diot_matter_devices";

static constexpr bool DEFAULT_POWER = true;
static constexpr uint8_t DEFAULT_BRIGHTNESS = 64;

static uint16_t s_light_endpoint_id = 0;
static diot_matter_led_t s_matter_led;

static esp_err_t log_white_temperature_request(const chip::app::ConcreteCommandPath &,
                                               chip::TLV::TLVReader &tlv_data, void *)
{
    ColorControl::Commands::MoveToColorTemperature::DecodableType request;
    CHIP_ERROR decode_error = chip::app::DataModel::Decode(tlv_data, request);
    if (decode_error == CHIP_NO_ERROR) {
        ESP_LOGI(TAG, "White requested: %u mired, transition: %u tenths of a second",
                 (unsigned)request.colorTemperatureMireds, (unsigned)request.transitionTime);
    } else {
        ESP_LOGW(TAG, "Could not decode White request: %" CHIP_ERROR_FORMAT, decode_error.Format());
    }
    return ESP_OK;
}

static esp_err_t apply_attribute_update(diot_matter_led_t *p_matter_led, uint16_t endpoint_id,
                                        uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    if (endpoint_id != s_light_endpoint_id) {
        return ESP_OK;
    }

    ESP_LOGW(TAG, "APP COMMAND -> Cluster: 0x%04lX, Attribute: 0x%04lX", cluster_id, attribute_id);

    if (cluster_id == OnOff::Id && attribute_id == OnOff::Attributes::OnOff::Id) {
        return diot_matter_led_set_on_off(p_matter_led, val->val.b);
    }

    if (cluster_id == LevelControl::Id && attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
        return diot_matter_led_set_brightness_from_matter(p_matter_led, val->val.u8);
    }

    if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            return diot_matter_led_set_hue_from_matter(p_matter_led, val->val.u8);
        }
        if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            return diot_matter_led_set_saturation_from_matter(p_matter_led, val->val.u8);
        }
        if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            ESP_LOGI(TAG, "White temperature: %u mired", (unsigned)val->val.u16);
            return diot_matter_led_set_temperature_mireds(p_matter_led, val->val.u16);
        }
        if (attribute_id == ColorControl::Attributes::CurrentX::Id) {
            return diot_matter_led_set_x(p_matter_led, val->val.u16);
        }
        if (attribute_id == ColorControl::Attributes::CurrentY::Id) {
            return diot_matter_led_set_y(p_matter_led, val->val.u16);
        }
    }

    return ESP_OK;
}

} // namespace

namespace diot::matter::devices {

esp_err_t create_light_device(node_t *node, diot_led_fsm_t *led_handle, uint16_t *light_endpoint_id)
{
    if (node == nullptr || led_handle == nullptr || light_endpoint_id == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = diot_matter_led_init(&s_matter_led, led_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize matter LED wrapper: %d", err);
        return err;
    }

    extended_color_light::config_t light_config;
    light_config.on_off.on_off = DEFAULT_POWER;
    light_config.on_off_lighting.start_up_on_off = nullptr;
    light_config.level_control.current_level = DEFAULT_BRIGHTNESS;
    light_config.level_control.on_level = DEFAULT_BRIGHTNESS;
    light_config.level_control_lighting.start_up_current_level = DEFAULT_BRIGHTNESS;
    light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation;
    light_config.color_control.enhanced_color_mode = (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation;
    light_config.color_control_color_temperature.color_temperature_mireds = 250;
    light_config.color_control_color_temperature.color_temp_physical_min_mireds = 153;
    light_config.color_control_color_temperature.color_temp_physical_max_mireds = 370;
    light_config.color_control_color_temperature.couple_color_temp_to_level_min_mireds = 153;
    light_config.color_control_color_temperature.start_up_color_temperature_mireds = nullptr;

    endpoint_t *light_endpoint = extended_color_light::create(node, &light_config, ENDPOINT_FLAG_NONE, &s_matter_led);
    if (light_endpoint == nullptr) {
        ESP_LOGE(TAG, "Failed to create extended color light endpoint");
        return ESP_FAIL;
    }

    cluster_t *color_control_cluster = cluster::get(light_endpoint, ColorControl::Id);
    cluster::color_control::feature::hue_saturation::config_t hue_saturation_config;
    err = cluster::color_control::feature::hue_saturation::add(color_control_cluster, &hue_saturation_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add Hue/Saturation feature: %d", err);
        return err;
    }

    command_t *white_command = command::get(color_control_cluster, ColorControl::Commands::MoveToColorTemperature::Id,
                                            COMMAND_FLAG_ACCEPTED);
    if (white_command == nullptr) {
        ESP_LOGE(TAG, "Missing MoveToColorTemperature command");
        return ESP_ERR_INVALID_STATE;
    }
    command::set_user_callback(white_command, log_white_temperature_request);

    *light_endpoint_id = endpoint::get_id(light_endpoint);
    s_light_endpoint_id = *light_endpoint_id;

    attribute_t *current_level_attribute =
        attribute::get(*light_endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::set_deferred_persistence(current_level_attribute);

    attribute_t *current_x_attribute =
        attribute::get(*light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
    attribute::set_deferred_persistence(current_x_attribute);
    attribute_t *current_y_attribute =
        attribute::get(*light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
    attribute::set_deferred_persistence(current_y_attribute);
    attribute_t *color_temp_attribute =
        attribute::get(*light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
    attribute::set_deferred_persistence(color_temp_attribute);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD && CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
    secondary_network_interface::config_t secondary_network_interface_config;
    endpoint_t *secondary_endpoint =
        endpoint::secondary_network_interface::create(node, &secondary_network_interface_config, ENDPOINT_FLAG_NONE, nullptr);
    if (secondary_endpoint == nullptr) {
        ESP_LOGE(TAG, "Failed to create secondary network interface endpoint");
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

esp_err_t apply_light_defaults(uint16_t light_endpoint_id)
{
    if (light_endpoint_id != s_light_endpoint_id) {
        return ESP_ERR_INVALID_ARG;
    }

    void *priv_data = endpoint::get_priv_data(light_endpoint_id);
    diot_matter_led_t *p_matter_led = (diot_matter_led_t *)priv_data;
    if (p_matter_led == nullptr) {
        ESP_LOGE(TAG, "Missing Matter LED private data for endpoint %u", light_endpoint_id);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_OK;
    esp_matter_attr_val_t val;

    attribute_t *attribute = attribute::get(light_endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::get_val(attribute, &val);
    err |= diot_matter_led_set_brightness_from_matter(p_matter_led, val.val.u8);

    attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorMode::Id);
    attribute::get_val(attribute, &val);
    if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation) {
        attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentHue::Id);
        attribute::get_val(attribute, &val);
        err |= diot_matter_led_set_hue_from_matter(p_matter_led, val.val.u8);
        attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentSaturation::Id);
        attribute::get_val(attribute, &val);
        err |= diot_matter_led_set_saturation_from_matter(p_matter_led, val.val.u8);
    } else if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kColorTemperature) {
        attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::get_val(attribute, &val);
        err |= diot_matter_led_set_temperature_mireds(p_matter_led, val.val.u16);
    } else if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kCurrentXAndCurrentY) {
        attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
        attribute::get_val(attribute, &val);
        err |= diot_matter_led_set_x(p_matter_led, val.val.u16);
        attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
        attribute::get_val(attribute, &val);
        err |= diot_matter_led_set_y(p_matter_led, val.val.u16);
    } else {
        ESP_LOGE(TAG, "Color mode not supported");
    }

    attribute = attribute::get(light_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= diot_matter_led_set_on_off(p_matter_led, val.val.b);

    return err;
}

esp_err_t identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant,
                            void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u endpoint: %u effect: %u variant: %u",
             type, endpoint_id, effect_id, effect_variant);
    (void)priv_data;
    return ESP_OK;
}

esp_err_t attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id,
                              esp_matter_attr_val_t *val, void *priv_data)
{
    if (type != attribute::PRE_UPDATE) {
        return ESP_OK;
    }

    if (val == nullptr || priv_data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    diot_matter_led_t *p_matter_led = (diot_matter_led_t *)priv_data;
    return apply_attribute_update(p_matter_led, endpoint_id, cluster_id, attribute_id, val);
}

} // namespace diot::matter::devices
