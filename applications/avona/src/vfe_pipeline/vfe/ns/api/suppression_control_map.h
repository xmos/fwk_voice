// Copyright (c) 2019, XMOS Ltd, All rights reserved
#ifndef SUPPRESSION_CONTROL_MAP_H_
#define SUPPRESSION_CONTROL_MAP_H_
#include "vtb_control.h"

typedef enum {
    // get: MSB == 1
    sup_cmd_get_bypass = 0x80,
    sup_cmd_get_echo_suppression_enabled,
    sup_cmd_get_noise_suppression_enabled,
    sup_cmd_get_noise_mcra_noise_floor,
    sup_num_get_commands,

    // set: MSB == 0
    sup_cmd_set_bypass = 0,
    sup_cmd_set_echo_suppression_enabled,
    sup_cmd_set_noise_suppression_enabled,
    sup_cmd_set_noise_mcra_noise_floor,
    sup_num_set_commands,
} sup_control_commands;

#define sup_num_commands ((sup_num_get_commands - 0x80) + sup_num_set_commands)
extern unsigned sup_control_map[sup_num_commands][3];

#endif // SUPPRESSION_CONTROL_MAP_H_
