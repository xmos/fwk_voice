// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef __DUMP_VAR_H__
#define __DUMP_VAR_H__


/*
 * This is designed for non-real time dumping of stuff
 * for reconstruction in python.
 */
void ic_dump_var_2d_start(ic_state_t *state, file_t *file_handle, unsigned n_frames);
void ic_dump_var_2d(ic_state_t *state);

void ic_dump_var_3d_start(ic_state_t *state, file_t *file_handle, unsigned n_frames);
void ic_dump_var_3d(ic_state_t *state);
#endif
