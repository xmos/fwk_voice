// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "testing.h"

#include <math.h>


/*
 * Type conversion
 */
double conv_s8_to_double (
    const int8_t x,  
    const exponent_t x_exp, 
    conv_error_e* error)
{
    //TODO check for errors
    return (double) ldexp((double)x, x_exp);
}

double conv_u8_to_double(
    const uint8_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    return (double) ldexp((double)x, x_exp);
}

double conv_s16_to_double(
    const int16_t x,  
    const exponent_t x_exp, 
    conv_error_e* error)
{
    //TODO check for errors
    return (double) ldexp((double)x, x_exp);
}

double conv_u16_to_double(
    const uint16_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    return (double) ldexp((double)x, x_exp);
}

double conv_s32_to_double (
    const int32_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    return (double) ldexp((double)x, x_exp);
}

double conv_u32_to_double(
    const uint32_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    return (double) ldexp((double)x, x_exp);
}

double conv_s64_to_double (
    const int64_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    return (double) ldexp((double)x, x_exp);
}

double conv_u64_to_double(
    const uint64_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    return (double) ldexp((double)x, x_exp);
}


int8_t  conv_double_to_s8 (
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = (double)frexp (d, &m_exp);

    double r = (double) ldexp(m, m_exp - d_exp);
    exponent_t output_exponent;
    frexp(r, &output_exponent);

    if(output_exponent>7){
        *error |= CANNOT_FIT_MANTISSA;
        return 0;
    }
    error = 0;
    return r;
}

uint8_t conv_double_to_u8(
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = (double)frexp (d, &m_exp);

    if(m<0.0){
        *error |= NEGATIVE_TO_UNSIGNED;
        return 0;
    }
    error = 0;
    return ldexp(m, m_exp - d_exp);
}

int16_t  conv_double_to_s16 (
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = (double)frexp (d, &m_exp);

    double r = (double) ldexp(m, m_exp - d_exp);
    exponent_t output_exponent;
    frexp(r, &output_exponent);

    if(output_exponent>15){
        *error |= CANNOT_FIT_MANTISSA;
        return 0;
    }
    error = 0;
    return r;
}

uint16_t conv_double_to_u16(
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = (double)frexp (d, &m_exp);

    if(m<0.0){
        *error |= NEGATIVE_TO_UNSIGNED;
        return 0;
    }

    error = 0;
    return ldexp(m, m_exp - d_exp);
}

int32_t  conv_double_to_s32 (
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = frexp (d, &m_exp);

    double r = ldexp(m, m_exp - d_exp);
    exponent_t output_exponent;
    frexp(r, &output_exponent);

    if(output_exponent>31){
        *error |= CANNOT_FIT_MANTISSA;
        return 0;
    }

    error = 0;

    return r;
}

uint32_t conv_double_to_u32(
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = frexp (d, &m_exp);

    if(m<0.0){
        *error |= NEGATIVE_TO_UNSIGNED;
        return 0;
    }
    error = 0;

    return ldexp(m, m_exp - d_exp);
}

int64_t  conv_double_to_s64 (
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = frexp (d, &m_exp);

    double r = ldexp(m, m_exp - d_exp);
    int output_exponent;
    frexp(r, &output_exponent);

    if(output_exponent>63){
        *error |= CANNOT_FIT_MANTISSA;
        return 0;
    }
    error = 0;

    return r;
}

uint64_t conv_double_to_u64(
    double d, 
    const exponent_t d_exp, 
    conv_error_e* error)
{
    exponent_t m_exp;
    double m = frexp (d, &m_exp);

    if(m<0.0){
        *error |= NEGATIVE_TO_UNSIGNED;
        return 0;
    }
    error = 0;

    return (uint64_t)ldexp(m, m_exp - d_exp);
}

complex_double_t conv_complex_s16_to_complex_double(
    complex_s16_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    complex_double_t r;
    r.re = conv_s16_to_double(x.re, x_exp, error);
    r.im = conv_s16_to_double(x.im, x_exp, error);

    return r;
}

complex_double_t conv_complex_s32_to_complex_double(
    complex_s32_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    complex_double_t r;
    r.re = conv_s32_to_double(x.re, x_exp, error);
    r.im = conv_s32_to_double(x.im, x_exp, error);

    return r;
}

complex_s16_t conv_complex_double_to_complex_s16(
    complex_double_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    complex_s16_t r;
    r.re = conv_double_to_s16(x.re, x_exp, error);
    r.im = conv_double_to_s16(x.im, x_exp, error);

    return r;
}

complex_s32_t conv_complex_double_to_complex_s32(
    complex_double_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    complex_s32_t r;
    r.re = conv_double_to_s32(x.re, x_exp, error);
    r.im = conv_double_to_s32(x.im, x_exp, error);

    return r;
}

complex_s64_t conv_complex_double_to_complex_s64(
    complex_double_t x, 
    const exponent_t x_exp, 
    conv_error_e* error)
{
    complex_s64_t r;
    r.re = conv_double_to_s64(x.re, x_exp, error);
    r.im = conv_double_to_s64(x.im, x_exp, error);

    return r;
}


/*
    Vector conversions
*/


void conv_vect_s16_to_double(
    double output[], 
    const int16_t input[], 
    const unsigned length, 
    const exponent_t input_exp, 
    conv_error_e *error)
{
    for(int i = 0; i < length; i++){
        output[i] = conv_s16_to_double(input[i], input_exp, error);
    }
}

void conv_vect_s32_to_double(
    double output[], 
    const int32_t input[], 
    const unsigned length, 
    const exponent_t input_exp, 
    conv_error_e *error)
{
    for(int i = 0; i < length; i++){
        output[i] = conv_s32_to_double(input[i], input_exp, error);
    }
}


void conv_vect_complex_s16_to_complex_double(
    complex_double_t output[], 
    const complex_s16_t input[], 
    const unsigned length, 
    const exponent_t input_exp, 
    conv_error_e *error)
{
    for(int i = 0; i < length; i++){
        output[i].re = conv_s16_to_double(input[i].re, input_exp, error);
        output[i].im = conv_s16_to_double(input[i].im, input_exp, error);
    }
}

void conv_vect_complex_s32_to_complex_double(
    complex_double_t output[], 
    const complex_s32_t input[], 
    const unsigned length, 
    const exponent_t input_exp, 
    conv_error_e *error)
{
    for(int i = 0; i < length; i++){
        output[i].re = conv_s32_to_double(input[i].re, input_exp, error);
        output[i].im = conv_s32_to_double(input[i].im, input_exp, error);
    }
}

void conv_vect_complex_s32_to_complex_double_v2(
    double out_real[], 
    double out_imag[], 
    const complex_s32_t input[], 
    const unsigned length, 
    const exponent_t input_exp, 
    conv_error_e *error)
{
    for(int i = 0; i < length; i++){
        out_real[i] = conv_s32_to_double(input[i].re, input_exp, error);
        out_imag[i] = conv_s32_to_double(input[i].im, input_exp, error);
    }
}