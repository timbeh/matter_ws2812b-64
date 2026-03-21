#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include "state/state.h"
#include "storage/storage.h"
#include "renderer/renderer.h"
#include "led_driver/led_driver.h"
#include "matter/matter_driver.h"

using namespace esp_matter;

static const char *TAG = "app_main";

static void app_event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;
    default:
        break;
    }
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;
    if (type == attribute::PRE_UPDATE) {
        err = matter_driver_attribute_update(NULL, endpoint_id, cluster_id, attribute_id, val);
    }
    return err;
}

extern "C" void app_main() {
    // 1. Storage and state init
    storage_init();
    LedState restored_state = storage_load_state();
    
    // Set state first so it can be restored
    state_set(restored_state);

    // 2. HW Driver Init
    led_driver_init();

    // 3. Renderer Init
    renderer_init();

    // 4. Initialize ESP-Matter
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);

    // Create an Extended Color Light endpoint
    using namespace chip::app::Clusters;
    endpoint::extended_color_light::config_t light_config;
    
    // Enable all color control features
    light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation;
    light_config.color_control.enhanced_color_mode = (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation;
    
    // Set default Color Temperature range (e.g., 2000K to 6500K)
    light_config.color_control_color_temperature.color_temp_physical_min_mireds = 153; 
    light_config.color_control_color_temperature.color_temp_physical_max_mireds = 500;
    
    endpoint_t *endpoint = endpoint::extended_color_light::create(
        node, &light_config, ENDPOINT_FLAG_NONE, NULL);

    // Manually add Hue/Saturation and Enhanced Hue features
    cluster_t *color_control_cluster = cluster::get(endpoint, ColorControl::Id);
    cluster::color_control::feature::hue_saturation::config_t hue_sat_config;
    cluster::color_control::feature::hue_saturation::add(color_control_cluster, &hue_sat_config);

    cluster::color_control::feature::enhanced_hue::config_t enhanced_hue_config;
    cluster::color_control::feature::enhanced_hue::add(color_control_cluster, &enhanced_hue_config);

    // Restore state to Matter attributes
    matter_driver_sync_attributes(1, restored_state);

    // Start Matter
    esp_matter::start(app_event_cb);
}