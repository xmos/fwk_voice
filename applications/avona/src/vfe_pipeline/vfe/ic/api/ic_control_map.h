// Copyright (c) 2019-2020, XMOS Ltd, All rights reserved
#ifndef IC_CONTROL_MAP_H_
#define IC_CONTROL_MAP_H_
#include "vtb_control.h"

// Size of a single get_filter_coefficients command in bytes
// Has to be < 64 (Limit of payload size using USB lib_device_control)
// and IC_COEFFICIENT_CHUNK_SIZE % 4 == 0 (Coefficients are uint32_t)
#define IC_COEFFICIENT_CHUNK_SIZE 56

typedef enum {
    // get: MSB == 1
    ic_cmd_get_bypass = 0x80,
    ic_cmd_get_x_energy_delta,
    ic_cmd_get_x_energy_gamma_log2,
    ic_cmd_get_forced_mu_value,
    ic_cmd_get_adaption_config,
    ic_cmd_get_sigma_alpha,
    ic_cmd_get_use_noise_minimisation,
    ic_cmd_get_phases,
    ic_cmd_get_proc_frame_bins,
    ic_cmd_get_filter_coefficients,
    ic_cmd_get_coefficient_index,
    ic_cmd_get_ch1_beamform_enable,
    ic_num_get_commands,

    // set: MSB == 0
    ic_cmd_set_bypass = 0,
    ic_cmd_set_x_energy_delta,
    ic_cmd_set_x_energy_gamma_log2,
    ic_cmd_set_forced_mu_value,
    ic_cmd_set_adaption_config,
    ic_cmd_set_sigma_alpha,
    ic_cmd_set_leakage_alpha,
    ic_cmd_set_use_noise_minimisation,
    ic_cmd_set_coefficient_index,
    ic_cmd_reset_filter,
    ic_cmd_set_ch1_beamform_enable,
    ic_num_set_commands,
} ic_control_commands;

#define ic_num_commands ((ic_num_get_commands - 0x80) + ic_num_set_commands)
extern unsigned ic_control_map[ic_num_commands][3];

#endif // ic_CONTROL_MAP_H
