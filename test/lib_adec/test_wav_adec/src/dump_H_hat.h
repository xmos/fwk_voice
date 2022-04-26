// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef __DUMP_H_HAT_H__
#define __DUMP_H_HAT_H__

/*
 * This is designed for non-real time dumping of the H_hat filter
 * for reconstruction in python.
 */
void aec_dump_H_hat(aec_state_t *state, file_t *file_handle);

#endif
