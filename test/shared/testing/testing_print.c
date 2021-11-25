// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "testing.h"

#include <math.h>

#include <stdio.h>


void print_vect_complex_s16(
    complex_s16_t * B, 
    const exponent_t B_exp,
    unsigned length,
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++){
        double re = conv_s16_to_double( B[i].re, B_exp, error);
        double im = conv_s16_to_double( B[i].im, B_exp, error);
        printf("%.*f + %.*fj, ", TESTING_FLOAT_NUM_DIGITS, re, TESTING_FLOAT_NUM_DIGITS, im);
    }
    printf("])\n");
}

void print_vect_complex_s16_fft(
    complex_s16_t * B, 
    const exponent_t B_exp,
    unsigned length,
    conv_error_e* error)
{
    printf("np.asarray([%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_s16_to_double( B[0].re, B_exp, error));
    for(unsigned i=1;i<length;i++){
        double re = conv_s16_to_double( B[i].re, B_exp, error);
        double im = conv_s16_to_double( B[i].im, B_exp, error);
        printf("%.*f + %.*fj, ", TESTING_FLOAT_NUM_DIGITS, re, TESTING_FLOAT_NUM_DIGITS, im);
    }
    printf("%.*f])\n", TESTING_FLOAT_NUM_DIGITS, conv_s16_to_double( B[0].im, B_exp, error));
}

void print_vect_complex_s32(
    complex_s32_t * B, 
    const exponent_t B_exp,
    unsigned length,
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++){
        double re = conv_s32_to_double( B[i].re, B_exp, error);
        double im = conv_s32_to_double( B[i].im, B_exp, error);
        printf("%.*f + %.*fj, ", TESTING_FLOAT_NUM_DIGITS, re, TESTING_FLOAT_NUM_DIGITS, im);
    }
    printf("])\n");
}

void print_vect_complex_s32_fft(
    complex_s32_t * B, 
    const exponent_t B_exp,
    unsigned length,
    conv_error_e* error)
{
    printf("np.asarray([%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_s32_to_double( B[0].re, B_exp, error));
    for(unsigned i=1;i<length;i++){
        double re = conv_s32_to_double( B[i].re, B_exp, error);
        double im = conv_s32_to_double( B[i].im, B_exp, error);
        printf("%.*f + %.*fj, ", TESTING_FLOAT_NUM_DIGITS, re, TESTING_FLOAT_NUM_DIGITS, im);
    }
    printf("%.*f])\n", TESTING_FLOAT_NUM_DIGITS, conv_s32_to_double(B[0].im, B_exp, error));
}

void print_vect_complex_s32_raw(
    complex_s32_t * B, 
    unsigned length)
{
    printf("np.asarray([");
    for(unsigned i = 0; i < length; i++){
        printf("%ld + %ldj, ", B[i].re, B[i].im);
    }
    printf("])\n");
}

void print_vect_complex_s32_fft_raw(
    complex_s32_t * B, 
    unsigned length)
{
    printf("np.asarray([%ld, ", B[0].re);
    for(unsigned i = 1; i < length; i++){
        printf("%ld + %ldj, ", B[i].re, B[i].im);
    }
    printf("%ld])\n", B[0].im);
}

void print_vect_complex_double(
    complex_double_t * B, 
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++){
        printf("%.*f + %.*fj, ", TESTING_FLOAT_NUM_DIGITS, B[i].re, TESTING_FLOAT_NUM_DIGITS, B[i].im);
    }
    printf("])\n");
}

void print_vect_complex_double_fft(
    complex_double_t * B, 
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([%.*f, ", TESTING_FLOAT_NUM_DIGITS, B[0].re);
    for(unsigned i=1;i<length;i++){
        printf("%.*f + %.*fj, ", TESTING_FLOAT_NUM_DIGITS, B[i].re, TESTING_FLOAT_NUM_DIGITS, B[i].im);
    }
    printf("%.*f])\n", TESTING_FLOAT_NUM_DIGITS, B[0].im);
}

void print_vect_s8 ( 
    int8_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_s8_to_double( B[i], B_exp, error));
    printf("])\n");
}

void print_vect_s16 ( 
    int16_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_s16_to_double( B[i], B_exp, error));
    printf("])\n");
}

void print_vect_s32 ( 
    int32_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_s32_to_double(B[i], B_exp, error));
    printf("])\n");
}

void print_vect_s64 ( 
    int64_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_s64_to_double(B[i], B_exp, error));
    printf("])\n");
}

void print_vect_u8(
    uint8_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_u8_to_double(B[i], B_exp, error));
    printf("])\n");
}

void print_vect_u16(
    uint16_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_u16_to_double(B[i], B_exp, error));
    printf("])\n");
}

void print_vect_u32(
    uint32_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_u32_to_double(B[i], B_exp, error));
    printf("])\n");
}

void print_vect_u64(
    uint64_t * B, 
    const exponent_t B_exp,
    unsigned length, 
    conv_error_e* error)
{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, conv_u64_to_double(B[i], B_exp, error));
    printf("])\n");
}

void print_vect_double (
    double * B, 
    unsigned length, 
    conv_error_e* error)

{
    printf("np.asarray([");
    for(unsigned i=0;i<length;i++)
        printf("%.*f, ", TESTING_FLOAT_NUM_DIGITS, B[i]);
    printf("])\n");
}

