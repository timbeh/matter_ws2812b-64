#include "app_priv.h"
#include "led_strip.h" // Required for the driver
#include <driver/rmt.h>
#include <esp_log.h>
#include <esp_matter.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace esp_matter;
using namespace chip::app::Clusters;

static const char *TAG = "app_driver";

// Hardware configuration
#define LED_STRIP_GPIO_PIN GPIO_NUM_8 // Change to your ESP32-C3 data pin
#define LED_STRIP_MAX_LEDS 64         // 64 LEDs for your WS2812B

static led_strip_t *led_strip;

// Store current state
static bool light_state = false;
static uint8_t light_level = 254; // Brightness
static uint8_t light_hue = 0;
static uint8_t light_saturation = 0;
static uint16_t light_x = 0;
static uint16_t light_y = 0;
static uint16_t light_mireds = 250;
static uint8_t light_color_mode = 0; // 0: HSV, 1: XY, 2: CT

// Helper function: Convert Matter HSV to standard RGB
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float h_f = (float)h * 360.0f / 254.0f;
    float s_f = (float)s / 254.0f;
    float v_f = (float)v / 254.0f;

    float c = v_f * s_f;
    float x = c * (1.0f - fabsf(fmodf(h_f / 60.0f, 2.0f) - 1.0f));
    float m = v_f - c;

    float r_f, g_f, b_f;
    if (h_f < 60) { r_f = c; g_f = x; b_f = 0; }
    else if (h_f < 120) { r_f = x; g_f = c; b_f = 0; }
    else if (h_f < 180) { r_f = 0; g_f = c; b_f = x; }
    else if (h_f < 240) { r_f = 0; g_f = x; b_f = c; }
    else if (h_f < 300) { r_f = x; g_f = 0; b_f = c; }
    else { r_f = c; g_f = 0; b_f = x; }

    *r = (uint8_t)((r_f + m) * 255.0f);
    *g = (uint8_t)((g_f + m) * 255.0f);
    *b = (uint8_t)((b_f + m) * 255.0f);
}

// Simple XY to RGB approximation
static void xy_to_rgb(uint16_t x, uint16_t y, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float x_f = (float)x / 65535.0f;
    float y_f = (float)y / 65535.0f;
    float z_f = 1.0f - x_f - y_f;

    // CIE XY to sRGB conversion (D65)
    float r_f = x_f * 3.2406f - y_f * 1.5372f - z_f * 0.4986f;
    float g_f = -x_f * 0.9689f + y_f * 1.8758f + z_f * 0.0415f;
    float b_f = x_f * 0.0557f - y_f * 0.2040f + z_f * 1.0570f;

    // Apply gamma correction (approximate)
    r_f = r_f <= 0.0031308f ? 12.92f * r_f : 1.055f * powf(r_f, 1.0f/2.4f) - 0.055f;
    g_f = g_f <= 0.0031308f ? 12.92f * g_f : 1.055f * powf(g_f, 1.0f/2.4f) - 0.055f;
    b_f = b_f <= 0.0031308f ? 12.92f * b_f : 1.055f * powf(b_f, 1.0f/2.4f) - 0.055f;

    float brightness = (float)v / 254.0f;
    *r = (uint8_t)(fmaxf(0, fminf(255, r_f * 255.0f)) * brightness);
    *g = (uint8_t)(fmaxf(0, fminf(255, g_f * 255.0f)) * brightness);
    *b = (uint8_t)(fmaxf(0, fminf(255, b_f * 255.0f)) * brightness);
}

// Mireds to RGB approximation
static void temp_to_rgb(uint16_t mireds, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    // Mireds to Kelvin
    uint32_t kelvin = 1000000 / mireds;
    float temp = kelvin / 100.0f;
    float red, green, blue;

    if (temp <= 66) {
        red = 255;
        green = temp;
        green = 99.4708025861f * logf(green) - 161.1195681661f;
        if (temp <= 19) {
            blue = 0;
        } else {
            blue = temp - 10;
            blue = 138.5177312231f * logf(blue) - 305.0447927307f;
        }
    } else {
        red = temp - 60;
        red = 329.698727446f * powf(red, -0.1332047592f);
        green = temp - 60;
        green = 288.1221695283f * powf(green, -0.0755148492f);
        blue = 255;
    }

    float brightness = (float)v / 254.0f;
    *r = (uint8_t)(fmaxf(0, fminf(255, red)) * brightness);
    *g = (uint8_t)(fmaxf(0, fminf(255, green)) * brightness);
    *b = (uint8_t)(fmaxf(0, fminf(255, blue)) * brightness);
}

// Update the physical LED strip
static esp_err_t app_driver_update_leds() {
  if (!light_state || light_level == 0) {
    if (led_strip) led_strip->clear(led_strip, 50);
    return ESP_OK;
  }

  uint8_t r, g, b;
  if (light_color_mode == 0) { // HSV
      hsv_to_rgb(light_hue, light_saturation, light_level, &r, &g, &b);
  } else if (light_color_mode == 1) { // XY
      xy_to_rgb(light_x, light_y, light_level, &r, &g, &b);
  } else { // CT
      temp_to_rgb(light_mireds, light_level, &r, &g, &b);
  }

  // WS2812B White Balance Correction
  // These LEDs typically have very strong Blue and Green channels
  // compared to Red. We scale them down to get a warmer, more natural white.
  // Tuning these values allows you to shift the "pure white" point.
  float wb_r = 1.0f;   // Red kept at 100%
  float wb_g = 0.75f;  // Green slightly reduced
  float wb_b = 0.50f;  // Blue drastically reduced to warm everything up

  uint8_t final_r = (uint8_t)(r * wb_r);
  uint8_t final_g = (uint8_t)(g * wb_g);
  uint8_t final_b = (uint8_t)(b * wb_b);

  ESP_LOGI(TAG, "Setting RGB: R=%d, G=%d, B=%d (Mode=%d) | Corrected: %d, %d, %d", r, g, b, light_color_mode, final_r, final_g, final_b);

  // Apply color to all 64 LEDs
  for (int i = 0; i < LED_STRIP_MAX_LEDS; i++) {
    // Some strips are GRB, some are RGB, some are BGR.
    // Based on user report: Red -> Blue, White -> Turquoise, Yellow -> Green
    // This suggests Arg 1 (R) maps to Blue, and maybe other swaps.
    // For now, let's keep it standard and use the log to diagnose.
    led_strip->set_pixel(led_strip, i, final_r, final_g, final_b);
  }

  return led_strip->refresh(led_strip, 50);
}

// Matter Attribute Update Callback
esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle,
                                      uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id,
                                      esp_matter_attr_val_t *val) {
  esp_err_t err = ESP_OK;

  if (endpoint_id != 1)
    return ESP_OK;

  if (cluster_id == OnOff::Id) {
    if (attribute_id == OnOff::Attributes::OnOff::Id) {
      light_state = val->val.b;
      ESP_LOGI(TAG, "New State: %d", light_state);
    }
  } else if (cluster_id == LevelControl::Id) {
    if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
      light_level = val->val.u8;
      ESP_LOGI(TAG, "New Level: %d", light_level);
    }
  } else if (cluster_id == ColorControl::Id) {
    if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
      light_hue = val->val.u8;
      light_color_mode = 0;
      ESP_LOGI(TAG, "New Hue: %d", light_hue);
    } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
      light_saturation = val->val.u8;
      light_color_mode = 0;
      ESP_LOGI(TAG, "New Sat: %d", light_saturation);
    } else if (attribute_id == ColorControl::Attributes::EnhancedCurrentHue::Id) {
      light_hue = (val->val.u16 * 254) / 65535;
      light_color_mode = 0;
      ESP_LOGI(TAG, "New Enhanced Hue: %d (mapped to %d)", val->val.u16, light_hue);
    } else if (attribute_id == ColorControl::Attributes::CurrentX::Id) {
      light_x = val->val.u16;
      light_color_mode = 1;
      ESP_LOGI(TAG, "New X: %d", light_x);
    } else if (attribute_id == ColorControl::Attributes::CurrentY::Id) {
      light_y = val->val.u16;
      light_color_mode = 1;
      ESP_LOGI(TAG, "New Y: %d", light_y);
    } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
      light_mireds = val->val.u16;
      light_color_mode = 2;
      ESP_LOGI(TAG, "New CT Mireds: %d", light_mireds);
    } else if (attribute_id == ColorControl::Attributes::ColorMode::Id) {
       ESP_LOGI(TAG, "New Color Mode: %d", val->val.u8);
    }
  }

  // Push changes to the WS2812B strip
  err = app_driver_update_leds();
  return err;
}

// Initialize the Hardware
esp_err_t app_driver_init() {
  ESP_LOGI(TAG, "Initializing WS2812B LED Strip");

  rmt_config_t rmt_cfg = RMT_DEFAULT_CONFIG_TX((gpio_num_t)LED_STRIP_GPIO_PIN, RMT_CHANNEL_0);
  rmt_cfg.clk_div = 2;
  ESP_ERROR_CHECK(rmt_config(&rmt_cfg));
  ESP_ERROR_CHECK(rmt_driver_install(rmt_cfg.channel, 0, 0));

  led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(LED_STRIP_MAX_LEDS, (led_strip_dev_t)rmt_cfg.channel);
  led_strip = led_strip_new_rmt_ws2812(&strip_config);
  
  if (!led_strip) {
      ESP_LOGE(TAG, "install WS2812 driver failed");
      return ESP_FAIL;
  }
  return ESP_OK;
}