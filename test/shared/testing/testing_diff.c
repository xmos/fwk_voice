// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "testing.h"

#include <math.h>


/*
 * Float/Fixed comparision
 */
unsigned abs_diff_complex_s16(
    complex_s16_t B, 
    const exponent_t B_exp, 
    complex_double_t f, 
    conv_error_e* error)
{
    unsigned re_diff = abs_diff_s16(B.re, B_exp, f.re, error);
    unsigned im_diff = abs_diff_s16(B.im, B_exp, f.im, error);
    if(re_diff > im_diff){
        return re_diff;
    } else {
        return im_diff;
    }
}

unsigned abs_diff_complex_s32(
    complex_s32_t B, 
    const exponent_t B_exp, 
    complex_double_t f, 
    conv_error_e* error)
{
    unsigned re_diff = abs_diff_s32(B.re, B_exp, f.re, error);
    unsigned im_diff = abs_diff_s32(B.im, B_exp, f.im, error);
    if(re_diff > im_diff){
        return re_diff;
    } else {
        return im_diff;
    }
}

unsigned abs_diff_s8 (
    int8_t B, 
    const exponent_t B_exp, 
    double f,  
    conv_error_e* error)
{
    int8_t v = conv_double_to_s8(f, B_exp, error);
    int diff = v - B;
    if (diff < 0 ) diff = -diff;
    return (unsigned)diff;
}

unsigned abs_diff_s16 ( 
    int16_t B, 
    const exponent_t B_exp, 
    double f,  
    conv_error_e* error)
{
    int16_t v = conv_double_to_s16(f, B_exp, error);
    int diff = v - B;
    if (diff < 0 ) diff = -diff;
    return (unsigned)diff;
}

unsigned abs_diff_s32 (
    int32_t B, 
    const exponent_t B_exp, 
    double f, 
    conv_error_e* error)
{
    int32_t v = conv_double_to_s32(f, B_exp, error);
    int diff = v - B;
    if (diff < 0 ) diff = -diff;
    return (unsigned)diff;
}

unsigned abs_diff_u8(
    uint8_t B, 
    const exponent_t B_exp, 
    double f, 
    conv_error_e* error)
{
    uint8_t v = conv_double_to_u8(f, B_exp, error);
    int diff = v - B;
    if (diff < 0 ) diff = -diff;
    return (unsigned)diff;
}

unsigned abs_diff_u16(
    uint16_t B, 
    const exponent_t B_exp, 
    double f, 
    conv_error_e* error)
{
    uint16_t v = conv_double_to_u16(f, B_exp, error);
    int diff = v - B;
    if (diff < 0 ) diff = -diff;
    return (unsigned)diff;
}

unsigned abs_diff_u32(
    uint32_t B, 
    const exponent_t B_exp, 
    double f, 
    conv_error_e* error)
{
    if ((B == 0) && (f != 0.0)){
        *error = 1;
        return 0;
    }
    uint32_t v = conv_double_to_u32(f, B_exp, error);
    int diff = v - B;
    if (diff < 0 ) diff = -diff;
    return (unsigned)diff;
}


/*
 * Float/Fixed vector comparision
 */

unsigned abs_diff_vect_complex_s16(
    complex_s16_t * B, 
    const exponent_t B_exp,
    complex_double_t * f, 
    unsigned length, 
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_complex_s16(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_complex_s32(
    complex_s32_t * B, 
    const exponent_t B_exp,
    complex_double_t * f, 
    unsigned length, 
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_complex_s32(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_s8 ( 
    int8_t * B, 
    const exponent_t B_exp, 
    double * f, 
    unsigned length,
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_s8(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_s16 ( 
    int16_t * B, 
    const exponent_t B_exp, 
    double * f, 
    unsigned length,
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_s16(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_s32 ( 
    int32_t * B, 
    const exponent_t B_exp, 
    double * f, 
    unsigned length,
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_s32(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_u8(
    uint8_t * B, 
    const exponent_t B_exp, 
    double * f, 
    unsigned length,
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_u8(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_u16(
    uint16_t * B, 
    const exponent_t B_exp, 
    double * f, 
    unsigned length,
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_u16(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

unsigned abs_diff_vect_u32(
    uint32_t * B, 
    const exponent_t B_exp, 
    double * f, 
    unsigned length,
    conv_error_e* error)
{
    unsigned max_diff = 0;

    for(unsigned i=0;i<length;i++){
        unsigned diff = abs_diff_u32(B[i], B_exp, f[i], error);
        if( diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}
