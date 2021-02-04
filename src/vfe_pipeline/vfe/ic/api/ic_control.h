// Copyright (c) 2018-2019, XMOS Ltd, All rights reserved
#ifndef IC_CONTROL_H_
#define IC_CONTROL_H_

#include "vtb_control.h"
#include "ic_state.h"
#include "ic_control_map.h"

void ic_command_handler(chanend c_command, ic_state_t &state);

void ic_get_bypass(ic_state_t &state, int &bypass);
void ic_get_x_energy_delta(ic_state_t &state, uint32_t &delta_m, uint32_t &delta_e);
void ic_get_x_energy_gamma_log2(ic_state_t &state, uint32_t &gamma_log2);
void ic_get_forced_mu_value(ic_state_t &state, uint32_t &mu);
void ic_get_adaption_config(ic_state_t &state, ic_adaption_e &adaption_config);
void ic_get_sigma_alpha(ic_state_t &state, uint32_t &sigma_xx_shift);
void ic_get_use_noise_minimisation(ic_state_t &state, int32_t &use_noise_minimisation);
void ic_get_phases(ic_state_t &state, uint32_t &phases);
void ic_get_proc_frame_bins(ic_state_t &state, uint32_t &proc_frame_bins);
void ic_get_filter_coefficients(ic_state_t &state, uint32_t chunk[]);
void ic_get_coefficient_index(ic_state_t &state, uint32_t &coeff_index);

void ic_set_bypass(ic_state_t &state, int bypass);
void ic_set_x_energy_delta(ic_state_t &state, uint32_t delta_m, uint32_t delta_e);
void ic_set_x_energy_gamma_log2(ic_state_t &state, uint32_t gamma_log2);
void ic_set_forced_mu_value(ic_state_t &state, uint32_t mu);
void ic_set_adaption_config(ic_state_t &state, ic_adaption_e adaption_config);
void ic_set_sigma_alpha(ic_state_t &state, uint32_t sigma_xx_shift);
void ic_set_leakage_alpha(ic_state_t &state, vtb_q0_31_t leakage_alpha);
void ic_set_use_noise_minimisation(ic_state_t &state, int use_noise_minimisation);
void ic_set_coefficient_index(ic_state_t &state, uint32_t coeff_index);

void ic_reset_filter(ic_state_t &state);

#endif /* IC_CONTROL_H_ */
