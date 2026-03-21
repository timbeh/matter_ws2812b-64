#pragma once

#include "state/state.h"

void renderer_init();
void renderer_enqueue_update(const LedState& new_state);
