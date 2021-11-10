// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef VTB_FLOAT_H_
#define VTB_FLOAT_H_

#include <xccompat.h>
#include <stdint.h>

#define VTB_FLOAT_ZERO_EXP (INT_MIN/2)
#define VTB_FLOAT_U32_ZERO {0, VTB_FLOAT_ZERO_EXP}
#define VTB_FLOAT_S32_ZERO {0, VTB_FLOAT_ZERO_EXP}
#define VTB_FLOAT_U32_ONE {UINT_MAX, -32}
#define VTB_FLOAT_S32_ONE {INT_MAX, -31}


/**
 * This defines the Q format of mu, the adaption coefficient for NLMS.
 * This will limit mu to [0.0, 2.0). The type of mu is always vtb_uq1_31_t.
 */
#define VTB_MU_EXP (-31)


/**
 * Floating point struct with S32 mantissa.
 */
typedef struct {
    int32_t m;  ///< Mantissa.
    int e;      ///< Exponent.
} vtb_s32_float_t;


/**
 * Floating point struct with U32 mantissa.
 */
typedef struct {
    uint32_t m; ///< Mantissa.
    int e;      ///< Exponent.
} vtb_u32_float_t;


/**
 * Floating point struct with S64 mantissa.
 */
typedef struct {
    int64_t m;  ///< Mantissa.
    int e;      ///< Exponent.
} vtb_s64_float_t;


/**
 * Floating point struct with S64 mantissa.
 */
typedef struct {
    uint64_t m; ///< Mantissa.
    int e;      ///< Exponent.
} vtb_u64_float_t;


/** Function that normalises a vtb_s32_float_t value.
 * A float value is considered normalised if the leading non-sign bit
 * of it's mantissa is 1.
 *
 * \param[in,out]  val  Floating point value to be normalised.
 *
 * \returns             No. of bit the mantissa was left-shifted by
 */
unsigned vtb_normalise_s32(REFERENCE_PARAM(vtb_s32_float_t, val));


/** Function that normalises a vtb_u32_float_t value.
 * A float value is considered normalised if the leading non-sign bit
 * of it's mantissa is 1.
 *
 * \param[in,out]  val  Floating point value to be normalised.
 *
 * \returns             No. of bit the mantissa was left-shifted by
 */
unsigned vtb_normalise_u32(REFERENCE_PARAM(vtb_u32_float_t,val));


/** Function that normalises a vtb_s64_float_t value.
 * A float value is considered normalised if the leading non-sign bit
 * of it's mantissa is 1.
 *
 * \param[in,out]  val  Floating point value to be normalised.
 *
 * \returns             No. of bit the mantissa was left-shifted by
 */
unsigned vtb_normalise_s64(REFERENCE_PARAM(vtb_s64_float_t, val));


/** Function that normalises a vtb_u64_float_t value.
 * A float value is considered normalised if the leading non-sign bit
 * of it's mantissa is 1.
 *
 * \param[in,out]  val  Floating point value to be normalised.
 *
 * \returns             No. of bit the mantissa was left-shifted by
 */
unsigned vtb_normalise_u64(REFERENCE_PARAM(vtb_u64_float_t, val));


/** Function that convers a vtb_u32_float_t to a vtb_s32_float_t value.
 *
 * \param[in]  a    vtb_u32_float_t value.
 *
 * \returns         Equivalent vtb_s32_float_t value (normalised).
 */
vtb_s32_float_t vtb_u32_to_s32(const vtb_u32_float_t a);


/** Function that convers a vtb_u64_float_t to a vtb_u32_float_t value.
 *  Assumes the input is normalised. Return value is normliased.
 *
 * \param[in]  a    vtb_u64_float_t value.
 *
 * \returns         Equivalent vtb_u32_float_t value (normalised).
 */
vtb_u32_float_t vtb_u64_to_u32(const vtb_u64_float_t a);


/** Function that convers a vtb_s64_float_t to a vtb_s32_float_t value.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    vtb_s64_float_t value.
 *
 * \returns         Equivalent vtb_s32_float_t value (normalised).
 */
vtb_s32_float_t vtb_s64_to_s32(const vtb_s64_float_t a);


/** Function that converts a vtb_u32_float_t value to uint32_t, given an exponent.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a            vtb_u32_float_t value to be converted.
 *
 * \param[in]  output_exp   Exponent of output.
 *
 * \returns                 Equivalent uint32_t value. Saturates at UINT_MAX.
 */
uint32_t vtb_denormalise_and_saturate_u32(const vtb_u32_float_t a, const int output_exp);


/** Function that converts a vtb_s32_float_t value to int32_t, given an exponent.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a            vtb_s32_float_t value to be converted.
 *
 * \param[in]  output_exp   Exponent of output.
 *
 * \returns                 Equivalent int32_t value. Saturates at INT_MAX.
 */
int32_t  vtb_denormalise_and_saturate_s32(const vtb_s32_float_t a, const int output_exp);


/** Function that returns the signed absolute value of a vtb_s32_float_t number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    Floating point value.
 *
 * \returns         Absolute value.
 */
vtb_s32_float_t vtb_abs_s32_to_s32(const vtb_s32_float_t a);


/** Function that returns unsigned absolute value of a vtb_s32_float_t number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    Floating point value.
 *
 * \returns         Absolute value.
 */
vtb_u32_float_t vtb_abs_s32_to_u32(const vtb_s32_float_t a);


/** Function that returns negative value of a vtb_s32_float_t number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    Floating point value.
 *
 * \returns         Negative value.
 */
vtb_s32_float_t vtb_neg_s32(const vtb_s32_float_t a);


/** Function that calculates if a floating point number is greater-than-or-equal
 *  to another floating point number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point number.
 *
 * \param[in]  b    Second floating point number.
 *
 * \returns         1 if first number is greater-than-or-equal to second number.
 *                  0 otherwise.
 */
int vtb_gte_s32_s32(const vtb_s32_float_t a, const vtb_s32_float_t b);


/** Function that calculates if a floating point number is greater-than-or-equal
 *  to another floating point number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point number.
 *
 * \param[in]  b    Second floating point number.
 *
 * \returns         1 if first number is greater-than-or-equal to second number.
 *                  0 otherwise.
 */
int vtb_gte_u32_u32(const vtb_u32_float_t a, const vtb_u32_float_t b);



/** Function that calculates if two signed floating point numbers are equal.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point number.
 *
 * \param[in]  b    Second floating point number.
 *
 * \returns         1 two inputs are equal. 0 otherwise.
 */
int vtb_eq_s32_s32(const vtb_s32_float_t a, const vtb_s32_float_t b);


/** Function that calculates if two unsigned floating point numbers are equal.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point number.
 *
 * \param[in]  b    Second floating point number.
 *
 * \returns         1 two inputs are equal. 0 otherwise.
 */
int vtb_eq_u32_u32(const vtb_u32_float_t a, const vtb_u32_float_t b);


/** Function that adds two signed floating point values.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         Sum of the two inputs.
 */
vtb_s32_float_t vtb_add_s32_s32(const vtb_s32_float_t a, const vtb_s32_float_t b);


/** Function that adds two unsigned floating point values.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         Sum of the two inputs.
 */
vtb_u32_float_t vtb_add_u32_u32(const vtb_u32_float_t a, const vtb_u32_float_t b);


/** Function that subtracts an unsigned floating point value from another.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a - b).
 */
vtb_s32_float_t vtb_sub_s32_s32(const vtb_s32_float_t a, const vtb_s32_float_t b);


/** Function that subtracts a signed floating point value from another.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a - b).
 */
vtb_s32_float_t vtb_sub_u32_u32(const vtb_u32_float_t a, const vtb_u32_float_t b);


/** Function that multiplies two signed floating point values.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a * b).
 */
vtb_s32_float_t vtb_mul_s32_s32(const vtb_s32_float_t a, const vtb_s32_float_t b);


/** Function that multiplies a signed and unsigned floating point value.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a * b).
 */
vtb_s32_float_t vtb_mul_s32_u32(const vtb_s32_float_t a, const vtb_u32_float_t b);


/** Function that multiplies two unsigned floating point values.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a * b).
 */
vtb_u32_float_t vtb_mul_u32_u32(const vtb_u32_float_t a, const vtb_u32_float_t b);


/** Function that divides a signed floating point number by another.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a / b).
 */
vtb_s32_float_t vtb_div_s32_s32(const vtb_s32_float_t a, const vtb_s32_float_t b);


/** Function that divides a signed floating point number by an
 *  unsigned floating point number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a / b).
 */
vtb_s32_float_t vtb_div_s32_u32(const vtb_s32_float_t a, const vtb_u32_float_t b);


/** Function that divides an unsigned floating point number by an
 *  unsigned floating point number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    First floating point value.
 *
 * \param[in]  b    Second floating point value.
 *
 * \returns         The result of (a / b).
 */
vtb_u32_float_t vtb_div_u32_u32(const vtb_u32_float_t a, const vtb_u32_float_t b);


/** Function that calculates the square root of an unsigned floating point number.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in]  a    Input floating point value.
 *
 * \returns         The square root of the input.
 */
vtb_u32_float_t vtb_sqrt_u32(const vtb_u32_float_t a);


/** Function that applies an exponential moving average.
 *  Assumes input is normalised. Return value is normliased.
 *
 * \param[in,out]  x        Current EWM value.
 *
 * \param[in]  new_sample   New sample value.
 *
 * \param[in]  alpha        Weighting coefficient.
 *
 * \returns  The result of (alpha * x + (1 - alpha) * new_sample).
 */
void vtb_exponential_average_u32(REFERENCE_PARAM(vtb_u32_float_t, x), vtb_u32_float_t new_sample, uint32_t alpha);


#endif /* VTB_FLOAT_H_ */
