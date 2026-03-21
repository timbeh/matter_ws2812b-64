#pragma once

#include "state/state.h"

void storage_init();
void storage_save_state(const LedState& state);
LedState storage_load_state();
