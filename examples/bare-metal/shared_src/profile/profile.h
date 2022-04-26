// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>

#define N_PROFILE_POINTS    100
/**
 * prof() function is called for profiling a piece of code. User needs to call it
 * in a specific way for it to work properly. 
 * If a piece of code needs to be profiled, call
 * prof(profiling_index_1, "start_xyz") and prof(profiling_index_2, "end_xyz") before and after
 * that piece of code. profiling_index_1 and profiling_index_2 are unique integers less than N_PROFILE_POINTS. 
 * xyz can be any string, as long as the same string is used prefixed with start_ and end_ for the 2 prof() calls. 
 * The start_ and end_ prefixes are important and need to be there.
 * profiling_indexes used in prof() calls need to be unique integers less than N_PROFILE_POINTS. The ordering
 * of these integers is not important.
 *
 * Following special cases are handled:
 * - Profiling code within a loop: prof() calls can be made for code within a loop. Only the total time taken by
 *   that code across all iterations will be calculated. Number or iterations is not saved anywhere and the 
 *   user needs to infer this separately if things like average cycles per iteration is needed.
 *
 * Following cases are NOT handled:
 * - For code within loops, the start_ and end_ prof() calls, both need to be either within the loop 
 *   or outside the loop. The start_ call within the loop and stop_ call outside the loop and vice-versa 
 *   are not supported.
 * - When profiling code within par statements, start_ and end_ calls cannot be within the par block.
 *   They need to be outside the par block.
 */
void prof(unsigned n, char *str);

/**
 * print_prof() is called once at the end of a frame to print the profiling info
 * collected in the prof() calls. Profiling info for valid profiling indexes between [start, end) is printed.
*/
void print_prof(unsigned start, unsigned end, unsigned frame_num);
