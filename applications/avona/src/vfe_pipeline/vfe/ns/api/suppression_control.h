// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef SUPPRESSION_CONTROL_H_
#define SUPPRESSION_CONTROL_H_

#include "dsp.h"
#include "vtb_control.h"
#include "suppression.h"
#include "suppression_state.h"
#include "suppression_control_map.h"

//TODO make these C callable
#ifdef __XC__
// Command handler
void sup_command_handler(chanend c_command, suppression_state_t &state);

// Getters
void sup_get_bypass(suppression_state_t &state, int &bypass);
void sup_get_echo_suppression_enabled(suppression_state_t &state, int &enabled);
void sup_get_noise_suppression_enabled(suppression_state_t &state, int &enabled);
void sup_get_noise_mcra_noise_floor(suppression_state_t &state, uint32_t &noise_floor);

// Setters
void sup_set_bypass(suppression_state_t &state, int bypass);
void sup_set_echo_suppression_enabled(suppression_state_t &state, int enabled);
void sup_set_noise_suppression_enabled(suppression_state_t &state, int enabled);
void sup_set_noise_mcra_noise_floor(suppression_state_t &state, uint32_t noise_floor);
#endif

#endif /* SUPPRESSION_CONTROL_H_ */
