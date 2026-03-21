#include "matter_driver.h"
#include <esp_log.h>
#include <esp_matter.h>

using namespace esp_matter;
using namespace chip::app::Clusters;

static const char *TAG = "matter_driver";

esp_err_t matter_driver_attribute_update(void *driver_handle, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) {
    if (endpoint_id != 1) return ESP_OK;

    if (cluster_id == OnOff::Id) {
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            ESP_LOGI(TAG, "New State: %d", val->val.b);
            state_update_onoff(val->val.b);
        }
    } else if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            ESP_LOGI(TAG, "New Level: %d", val->val.u8);
            state_update_level(val->val.u8);
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            ESP_LOGI(TAG, "New Hue: %d", val->val.u8);
            state_update_hue(val->val.u8);
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            ESP_LOGI(TAG, "New Sat: %d", val->val.u8);
            state_update_saturation(val->val.u8);
        } else if (attribute_id == ColorControl::Attributes::EnhancedCurrentHue::Id) {
            ESP_LOGI(TAG, "New Enhanced Hue: %d", val->val.u16);
            state_update_enhanced_hue(val->val.u16);
        } else if (attribute_id == ColorControl::Attributes::CurrentX::Id) {
            ESP_LOGI(TAG, "New X: %d", val->val.u16);
            state_update_xy(val->val.u16, state_get().y);
        } else if (attribute_id == ColorControl::Attributes::CurrentY::Id) {
            ESP_LOGI(TAG, "New Y: %d", val->val.u16);
            state_update_xy(state_get().x, val->val.u16);
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            ESP_LOGI(TAG, "New CT Mireds: %d", val->val.u16);
            state_update_ct(val->val.u16);
        }
    }
    return ESP_OK;
}

void matter_driver_sync_attributes(uint16_t endpoint_id, const LedState& current_state) {
    node_t *node = node::get();
    if (!node) return;
    endpoint_t *ep = endpoint::get(node, endpoint_id);
    if (!ep) return;

    {
        cluster_t *c = cluster::get(ep, OnOff::Id);
        if (c) {
            attribute_t *a = attribute::get(c, OnOff::Attributes::OnOff::Id);
            if (a) {
                esp_matter_attr_val_t val = esp_matter_bool(current_state.on);
                attribute::update(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
            }
        }
    }

    {
        cluster_t *c = cluster::get(ep, LevelControl::Id);
        if (c) {
            attribute_t *a = attribute::get(c, LevelControl::Attributes::CurrentLevel::Id);
            if (a) {
                esp_matter_attr_val_t val = esp_matter_uint8(current_state.brightness);
                attribute::update(endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id, &val);
            }
        }
    }

    {
        cluster_t *c = cluster::get(ep, ColorControl::Id);
        if (c) {
            attribute_t *a_mode = attribute::get(c, ColorControl::Attributes::ColorMode::Id);
            if (a_mode) {
                esp_matter_attr_val_t val = esp_matter_enum8(current_state.color_mode);
                attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorMode::Id, &val);
            }
            if (current_state.color_mode == 0) {
                 attribute_t *a_hue = attribute::get(c, ColorControl::Attributes::CurrentHue::Id);
                 if (a_hue) {
                     esp_matter_attr_val_t val = esp_matter_uint8(current_state.hue);
                     attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentHue::Id, &val);
                 }
                 attribute_t *a_sat = attribute::get(c, ColorControl::Attributes::CurrentSaturation::Id);
                 if (a_sat) {
                     esp_matter_attr_val_t val = esp_matter_uint8(current_state.saturation);
                     attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentSaturation::Id, &val);
                 }
            } else if (current_state.color_mode == 1) {
                 attribute_t *a_x = attribute::get(c, ColorControl::Attributes::CurrentX::Id);
                 if (a_x) {
                     esp_matter_attr_val_t val = esp_matter_uint16(current_state.x);
                     attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id, &val);
                 }
                 attribute_t *a_y = attribute::get(c, ColorControl::Attributes::CurrentY::Id);
                 if (a_y) {
                     esp_matter_attr_val_t val = esp_matter_uint16(current_state.y);
                     attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id, &val);
                 }
            } else if (current_state.color_mode == 2) {
                 attribute_t *a_ct = attribute::get(c, ColorControl::Attributes::ColorTemperatureMireds::Id);
                 if (a_ct) {
                     esp_matter_attr_val_t val = esp_matter_uint16(current_state.mireds);
                     attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id, &val);
                 }
            }
        }
    }
}
