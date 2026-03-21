#include "state.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include "renderer/renderer.h"
#include "storage/storage.h"

static const char *TAG = "state";

static LedState current_state = {
    .on = false,
    .brightness = 254,
    .hue = 0,
    .saturation = 0,
    .x = 0,
    .y = 0,
    .mireds = 250,
    .color_mode = 0
};

static SemaphoreHandle_t state_mutex = nullptr;

static void ensure_mutex() {
    if (!state_mutex) {
        state_mutex = xSemaphoreCreateMutex();
    }
}

LedState state_get() {
    ensure_mutex();
    LedState s;
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    s = current_state;
    xSemaphoreGive(state_mutex);
    return s;
}

void state_set(const LedState& new_state) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    current_state = new_state;
    xSemaphoreGive(state_mutex);
    
    // Store persistently config and enqueue rendering
    storage_save_state(current_state);
    renderer_enqueue_update(current_state);
}

void state_update_onoff(bool on) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    bool changed = current_state.on != on;
    current_state.on = on;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}

void state_update_level(uint8_t level) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    bool changed = current_state.brightness != level;
    current_state.brightness = level;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}

void state_update_hue(uint8_t hue) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    bool changed = (current_state.hue != hue) || (current_state.color_mode != 0);
    current_state.hue = hue;
    current_state.color_mode = 0;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}

void state_update_enhanced_hue(uint16_t hue) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    uint8_t mapped_hue = (hue * 254) / 65535;
    bool changed = (current_state.hue != mapped_hue) || (current_state.color_mode != 0);
    current_state.hue = mapped_hue;
    current_state.color_mode = 0;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}

void state_update_saturation(uint8_t sat) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    bool changed = (current_state.saturation != sat) || (current_state.color_mode != 0);
    current_state.saturation = sat;
    current_state.color_mode = 0;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}

void state_update_xy(uint16_t x, uint16_t y) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    bool changed = (current_state.x != x || current_state.y != y) || (current_state.color_mode != 1);
    current_state.x = x;
    current_state.y = y;
    current_state.color_mode = 1;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}

void state_update_ct(uint16_t mireds) {
    ensure_mutex();
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    bool changed = (current_state.mireds != mireds) || (current_state.color_mode != 2);
    current_state.mireds = mireds;
    current_state.color_mode = 2;
    LedState s = current_state;
    xSemaphoreGive(state_mutex);
    if (changed) {
        storage_save_state(s);
        renderer_enqueue_update(s);
    }
}
