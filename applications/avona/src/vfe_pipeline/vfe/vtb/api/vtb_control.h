// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef _VTB_CONTROL_H_
#define _VTB_CONTROL_H_

#include <stdint.h>

typedef enum {
    vtb_ctrl_uint64,
    vtb_ctrl_uint32,
    vtb_ctrl_int32,
    vtb_ctrl_fixed_0_32,
    vtb_ctrl_fixed_1_31,
    vtb_ctrl_fixed_8_24,
    vtb_ctrl_fixed_16_16,
    vtb_ctrl_uint8
} vtb_control_type_t;

#ifdef __XC__
int vtb_control_is_get_cmd(uint8_t cmd);
int vtb_control_is_set_cmd(uint8_t cmd);

int vtb_control_type(const unsigned control_map[][3], unsigned num_commands,
                     uint8_t cmd);
int vtb_control_num_vals(const unsigned control_map[][3], unsigned num_commands,
                         uint8_t cmd);

int vtb_control_sizeof(vtb_control_type_t type);

void vtb_control_get_n_bytes(chanend c_cmd, unsigned n, uint8_t vals[n]);
void vtb_control_get_n_words(chanend c_cmd, unsigned n, uint32_t vals[n]);
void vtb_control_get_n_double_words(chanend c_cmd, unsigned n, uint64_t vals[n]);
void vtb_control_set_n_bytes(chanend c_cmd, unsigned n, uint8_t vals[n]);
void vtb_control_set_n_words(chanend c_cmd, unsigned n, uint32_t vals[n]);
void vtb_control_set_n_double_words(chanend c_cmd, unsigned n, uint64_t vals[n]);

void vtb_control_reset(chanend c_cmd);

void vtb_control_handle_get_n_double_words(chanend c_cmd, unsigned n, uint64_t vals[n]);
void vtb_control_handle_get_double_word(chanend c_cmd, uint64_t vals);
void vtb_control_handle_get_n_words(chanend c_cmd, unsigned n, uint32_t vals[n]);
void vtb_control_handle_get_word(chanend c_cmd, uint32_t vals);
void vtb_control_handle_get_n_bytes(chanend c_cmd, unsigned n, uint8_t vals[n]);
void vtb_control_handle_get_byte(chanend c_cmd, uint8_t vals);

int vtb_control_handle_set_double_word(chanend c_cmd, uint64_t &val);
int vtb_control_handle_set_n_double_words(chanend c_cmd, unsigned n, uint64_t vals[n]);
int vtb_control_handle_set_word(chanend c_cmd, uint32_t &val);
int vtb_control_handle_set_n_words(chanend c_cmd, unsigned n, uint32_t vals[n]);
int vtb_control_handle_set_byte(chanend c_cmd, uint8_t &val);
int vtb_control_handle_set_n_bytes(chanend c_cmd, unsigned n, uint8_t vals[n]);

int vtb_control_handle_reset(chanend c_cmd);
#endif // __XC__

#endif // _VTB_CONTROL_H_
