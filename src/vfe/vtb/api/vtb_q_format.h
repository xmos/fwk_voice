// Copyright (c) 2019-2020, XMOS Ltd, All rights reserved
#include <stdint.h>

#ifndef VTB_Q_FORMAT_H_
#define VTB_Q_FORMAT_H_

/** Represents: 0.0 to 0.999999999767 in 2^-32 steps
 */
typedef uint32_t vtb_uq0_32_t; 
/** Represents: 0.0 to 1.99999999953 in 2^-32 steps
 */
typedef uint32_t vtb_uq1_31_t; 
/** Represents: 0.0 to 3.99999999907 in 2^-32 steps
 */
typedef uint32_t vtb_uq2_30_t; 
/** Represents: 0.0 to 7.99999999814 in 2^-32 steps
 */
typedef uint32_t vtb_uq3_29_t; 
/** Represents: 0.0 to 15.9999999963 in 2^-32 steps
 */
typedef uint32_t vtb_uq4_28_t; 
/** Represents: 0.0 to 31.9999999925 in 2^-32 steps
 */
typedef uint32_t vtb_uq5_27_t; 
/** Represents: 0.0 to 63.9999999851 in 2^-32 steps
 */
typedef uint32_t vtb_uq6_26_t; 
/** Represents: 0.0 to 127.99999997 in 2^-32 steps
 */
typedef uint32_t vtb_uq7_25_t; 
/** Represents: 0.0 to 255.99999994 in 2^-32 steps
 */
typedef uint32_t vtb_uq8_24_t; 
/** Represents: 0.0 to 511.999999881 in 2^-32 steps
 */
typedef uint32_t vtb_uq9_23_t; 
/** Represents: 0.0 to 1023.99999976 in 2^-32 steps
 */
typedef uint32_t vtb_uq10_22_t; 
/** Represents: 0.0 to 2047.99999952 in 2^-32 steps
 */
typedef uint32_t vtb_uq11_21_t; 
/** Represents: 0.0 to 4095.99999905 in 2^-32 steps
 */
typedef uint32_t vtb_uq12_20_t; 
/** Represents: 0.0 to 8191.99999809 in 2^-32 steps
 */
typedef uint32_t vtb_uq13_19_t; 
/** Represents: 0.0 to 16383.9999962 in 2^-32 steps
 */
typedef uint32_t vtb_uq14_18_t; 
/** Represents: 0.0 to 32767.9999924 in 2^-32 steps
 */
typedef uint32_t vtb_uq15_17_t; 
/** Represents: 0.0 to 65535.9999847 in 2^-32 steps
 */
typedef uint32_t vtb_uq16_16_t; 
/** Represents: 0.0 to 131071.999969 in 2^-32 steps
 */
typedef uint32_t vtb_uq17_15_t; 
/** Represents: 0.0 to 262143.999939 in 2^-32 steps
 */
typedef uint32_t vtb_uq18_14_t; 
/** Represents: 0.0 to 524287.999878 in 2^-32 steps
 */
typedef uint32_t vtb_uq19_13_t; 
/** Represents: 0.0 to 1048575.99976 in 2^-32 steps
 */
typedef uint32_t vtb_uq20_12_t; 
/** Represents: 0.0 to 2097151.99951 in 2^-32 steps
 */
typedef uint32_t vtb_uq21_11_t; 
/** Represents: 0.0 to 4194303.99902 in 2^-32 steps
 */
typedef uint32_t vtb_uq22_10_t; 
/** Represents: 0.0 to 8388607.99805 in 2^-32 steps
 */
typedef uint32_t vtb_uq23_9_t; 
/** Represents: 0.0 to 16777215.9961 in 2^-32 steps
 */
typedef uint32_t vtb_uq24_8_t; 
/** Represents: 0.0 to 33554431.9922 in 2^-32 steps
 */
typedef uint32_t vtb_uq25_7_t; 
/** Represents: 0.0 to 67108863.9844 in 2^-32 steps
 */
typedef uint32_t vtb_uq26_6_t; 
/** Represents: 0.0 to 134217727.969 in 2^-32 steps
 */
typedef uint32_t vtb_uq27_5_t; 
/** Represents: 0.0 to 268435455.938 in 2^-32 steps
 */
typedef uint32_t vtb_uq28_4_t; 
/** Represents: 0.0 to 536870911.875 in 2^-32 steps
 */
typedef uint32_t vtb_uq29_3_t; 
/** Represents: 0.0 to 1073741823.75 in 2^-32 steps
 */
typedef uint32_t vtb_uq30_2_t; 
/** Represents: 0.0 to 2147483647.5 in 2^-32 steps
 */
typedef uint32_t vtb_uq31_1_t; 
/** Represents: 0.0 to 4294967295.0 in 2^-32 steps
 */
typedef uint32_t vtb_uq32_0_t; 

/** Represents: -1.0 to 0.999999999534 in 2^-32 steps
 */
typedef int32_t vtb_q0_31_t; 
/** Represents: -2.0 to 1.99999999907 in 2^-32 steps
 */
typedef int32_t vtb_q1_30_t; 
/** Represents: -4.0 to 3.99999999814 in 2^-32 steps
 */
typedef int32_t vtb_q2_29_t; 
/** Represents: -8.0 to 7.99999999627 in 2^-32 steps
 */
typedef int32_t vtb_q3_28_t; 
/** Represents: -16.0 to 15.9999999925 in 2^-32 steps
 */
typedef int32_t vtb_q4_27_t; 
/** Represents: -32.0 to 31.9999999851 in 2^-32 steps
 */
typedef int32_t vtb_q5_26_t; 
/** Represents: -64.0 to 63.9999999702 in 2^-32 steps
 */
typedef int32_t vtb_q6_25_t; 
/** Represents: -128.0 to 127.99999994 in 2^-32 steps
 */
typedef int32_t vtb_q7_24_t; 
/** Represents: -256.0 to 255.999999881 in 2^-32 steps
 */
typedef int32_t vtb_q8_23_t; 
/** Represents: -512.0 to 511.999999762 in 2^-32 steps
 */
typedef int32_t vtb_q9_22_t; 
/** Represents: -1024.0 to 1023.99999952 in 2^-32 steps
 */
typedef int32_t vtb_q10_21_t; 
/** Represents: -2048.0 to 2047.99999905 in 2^-32 steps
 */
typedef int32_t vtb_q11_20_t; 
/** Represents: -4096.0 to 4095.99999809 in 2^-32 steps
 */
typedef int32_t vtb_q12_19_t; 
/** Represents: -8192.0 to 8191.99999619 in 2^-32 steps
 */
typedef int32_t vtb_q13_18_t; 
/** Represents: -16384.0 to 16383.9999924 in 2^-32 steps
 */
typedef int32_t vtb_q14_17_t; 
/** Represents: -32768.0 to 32767.9999847 in 2^-32 steps
 */
typedef int32_t vtb_q15_16_t; 
/** Represents: -65536.0 to 65535.9999695 in 2^-32 steps
 */
typedef int32_t vtb_q16_15_t; 
/** Represents: -131072.0 to 131071.999939 in 2^-32 steps
 */
typedef int32_t vtb_q17_14_t; 
/** Represents: -262144.0 to 262143.999878 in 2^-32 steps
 */
typedef int32_t vtb_q18_13_t; 
/** Represents: -524288.0 to 524287.999756 in 2^-32 steps
 */
typedef int32_t vtb_q19_12_t; 
/** Represents: -1048576.0 to 1048575.99951 in 2^-32 steps
 */
typedef int32_t vtb_q20_11_t; 
/** Represents: -2097152.0 to 2097151.99902 in 2^-32 steps
 */
typedef int32_t vtb_q21_10_t; 
/** Represents: -4194304.0 to 4194303.99805 in 2^-32 steps
 */
typedef int32_t vtb_q22_9_t; 
/** Represents: -8388608.0 to 8388607.99609 in 2^-32 steps
 */
typedef int32_t vtb_q23_8_t; 
/** Represents: -16777216.0 to 16777215.9922 in 2^-32 steps
 */
typedef int32_t vtb_q24_7_t; 
/** Represents: -33554432.0 to 33554431.9844 in 2^-32 steps
 */
typedef int32_t vtb_q25_6_t; 
/** Represents: -67108864.0 to 67108863.9688 in 2^-32 steps
 */
typedef int32_t vtb_q26_5_t; 
/** Represents: -134217728.0 to 134217727.938 in 2^-32 steps
 */
typedef int32_t vtb_q27_4_t; 
/** Represents: -268435456.0 to 268435455.875 in 2^-32 steps
 */
typedef int32_t vtb_q28_3_t; 
/** Represents: -536870912.0 to 536870911.75 in 2^-32 steps
 */
typedef int32_t vtb_q29_2_t; 
/** Represents: -1073741824.0 to 1073741823.5 in 2^-32 steps
 */
typedef int32_t vtb_q30_1_t; 
/** Represents: -2147483648.0 to 2147483647.0 in 2^-32 steps
 */
typedef int32_t vtb_q31_0_t; 

/** Casts a double to a vtb_uq0_32_t.
 */
#define VTB_UQ0_32(f) ((vtb_uq0_32_t)((double)(UINT_MAX>>0) * f))
/** Casts a double to a vtb_uq1_31_t.
 */
#define VTB_UQ1_31(f) ((vtb_uq1_31_t)((double)(UINT_MAX>>1) * f))
/** Casts a double to a vtb_uq2_30_t.
 */
#define VTB_UQ2_30(f) ((vtb_uq2_30_t)((double)(UINT_MAX>>2) * f))
/** Casts a double to a vtb_uq3_29_t.
 */
#define VTB_UQ3_29(f) ((vtb_uq3_29_t)((double)(UINT_MAX>>3) * f))
/** Casts a double to a vtb_uq4_28_t.
 */
#define VTB_UQ4_28(f) ((vtb_uq4_28_t)((double)(UINT_MAX>>4) * f))
/** Casts a double to a vtb_uq5_27_t.
 */
#define VTB_UQ5_27(f) ((vtb_uq5_27_t)((double)(UINT_MAX>>5) * f))
/** Casts a double to a vtb_uq6_26_t.
 */
#define VTB_UQ6_26(f) ((vtb_uq6_26_t)((double)(UINT_MAX>>6) * f))
/** Casts a double to a vtb_uq7_25_t.
 */
#define VTB_UQ7_25(f) ((vtb_uq7_25_t)((double)(UINT_MAX>>7) * f))
/** Casts a double to a vtb_uq8_24_t.
 */
#define VTB_UQ8_24(f) ((vtb_uq8_24_t)((double)(UINT_MAX>>8) * f))
/** Casts a double to a vtb_uq9_23_t.
 */
#define VTB_UQ9_23(f) ((vtb_uq9_23_t)((double)(UINT_MAX>>9) * f))
/** Casts a double to a vtb_uq10_22_t.
 */
#define VTB_UQ10_22(f) ((vtb_uq10_22_t)((double)(UINT_MAX>>10) * f))
/** Casts a double to a vtb_uq11_21_t.
 */
#define VTB_UQ11_21(f) ((vtb_uq11_21_t)((double)(UINT_MAX>>11) * f))
/** Casts a double to a vtb_uq12_20_t.
 */
#define VTB_UQ12_20(f) ((vtb_uq12_20_t)((double)(UINT_MAX>>12) * f))
/** Casts a double to a vtb_uq13_19_t.
 */
#define VTB_UQ13_19(f) ((vtb_uq13_19_t)((double)(UINT_MAX>>13) * f))
/** Casts a double to a vtb_uq14_18_t.
 */
#define VTB_UQ14_18(f) ((vtb_uq14_18_t)((double)(UINT_MAX>>14) * f))
/** Casts a double to a vtb_uq15_17_t.
 */
#define VTB_UQ15_17(f) ((vtb_uq15_17_t)((double)(UINT_MAX>>15) * f))
/** Casts a double to a vtb_uq16_16_t.
 */
#define VTB_UQ16_16(f) ((vtb_uq16_16_t)((double)(UINT_MAX>>16) * f))
/** Casts a double to a vtb_uq17_15_t.
 */
#define VTB_UQ17_15(f) ((vtb_uq17_15_t)((double)(UINT_MAX>>17) * f))
/** Casts a double to a vtb_uq18_14_t.
 */
#define VTB_UQ18_14(f) ((vtb_uq18_14_t)((double)(UINT_MAX>>18) * f))
/** Casts a double to a vtb_uq19_13_t.
 */
#define VTB_UQ19_13(f) ((vtb_uq19_13_t)((double)(UINT_MAX>>19) * f))
/** Casts a double to a vtb_uq20_12_t.
 */
#define VTB_UQ20_12(f) ((vtb_uq20_12_t)((double)(UINT_MAX>>20) * f))
/** Casts a double to a vtb_uq21_11_t.
 */
#define VTB_UQ21_11(f) ((vtb_uq21_11_t)((double)(UINT_MAX>>21) * f))
/** Casts a double to a vtb_uq22_10_t.
 */
#define VTB_UQ22_10(f) ((vtb_uq22_10_t)((double)(UINT_MAX>>22) * f))
/** Casts a double to a vtb_uq23_9_t.
 */
#define VTB_UQ23_9(f) ((vtb_uq23_9_t)((double)(UINT_MAX>>23) * f))
/** Casts a double to a vtb_uq24_8_t.
 */
#define VTB_UQ24_8(f) ((vtb_uq24_8_t)((double)(UINT_MAX>>24) * f))
/** Casts a double to a vtb_uq25_7_t.
 */
#define VTB_UQ25_7(f) ((vtb_uq25_7_t)((double)(UINT_MAX>>25) * f))
/** Casts a double to a vtb_uq26_6_t.
 */
#define VTB_UQ26_6(f) ((vtb_uq26_6_t)((double)(UINT_MAX>>26) * f))
/** Casts a double to a vtb_uq27_5_t.
 */
#define VTB_UQ27_5(f) ((vtb_uq27_5_t)((double)(UINT_MAX>>27) * f))
/** Casts a double to a vtb_uq28_4_t.
 */
#define VTB_UQ28_4(f) ((vtb_uq28_4_t)((double)(UINT_MAX>>28) * f))
/** Casts a double to a vtb_uq29_3_t.
 */
#define VTB_UQ29_3(f) ((vtb_uq29_3_t)((double)(UINT_MAX>>29) * f))
/** Casts a double to a vtb_uq30_2_t.
 */
#define VTB_UQ30_2(f) ((vtb_uq30_2_t)((double)(UINT_MAX>>30) * f))
/** Casts a double to a vtb_uq31_1_t.
 */
#define VTB_UQ31_1(f) ((vtb_uq31_1_t)((double)(UINT_MAX>>31) * f))
/** Casts a double to a vtb_uq32_0_t.
 */
#define VTB_UQ32_0(f) ((vtb_uq32_0_t)((double)(UINT_MAX>>32) * f))

/** Casts a double to a vtb_q0_31_t.
 */
#define VTB_Q0_31(f) ((vtb_q0_31_t)((double)(INT_MAX>>0) * f))
/** Casts a double to a vtb_q1_30_t.
 */
#define VTB_Q1_30(f) ((vtb_q1_30_t)((double)(INT_MAX>>1) * f))
/** Casts a double to a vtb_q2_29_t.
 */
#define VTB_Q2_29(f) ((vtb_q2_29_t)((double)(INT_MAX>>2) * f))
/** Casts a double to a vtb_q3_28_t.
 */
#define VTB_Q3_28(f) ((vtb_q3_28_t)((double)(INT_MAX>>3) * f))
/** Casts a double to a vtb_q4_27_t.
 */
#define VTB_Q4_27(f) ((vtb_q4_27_t)((double)(INT_MAX>>4) * f))
/** Casts a double to a vtb_q5_26_t.
 */
#define VTB_Q5_26(f) ((vtb_q5_26_t)((double)(INT_MAX>>5) * f))
/** Casts a double to a vtb_q6_25_t.
 */
#define VTB_Q6_25(f) ((vtb_q6_25_t)((double)(INT_MAX>>6) * f))
/** Casts a double to a vtb_q7_24_t.
 */
#define VTB_Q7_24(f) ((vtb_q7_24_t)((double)(INT_MAX>>7) * f))
/** Casts a double to a vtb_q8_23_t.
 */
#define VTB_Q8_23(f) ((vtb_q8_23_t)((double)(INT_MAX>>8) * f))
/** Casts a double to a vtb_q9_22_t.
 */
#define VTB_Q9_22(f) ((vtb_q9_22_t)((double)(INT_MAX>>9) * f))
/** Casts a double to a vtb_q10_21_t.
 */
#define VTB_Q10_21(f) ((vtb_q10_21_t)((double)(INT_MAX>>10) * f))
/** Casts a double to a vtb_q11_20_t.
 */
#define VTB_Q11_20(f) ((vtb_q11_20_t)((double)(INT_MAX>>11) * f))
/** Casts a double to a vtb_q12_19_t.
 */
#define VTB_Q12_19(f) ((vtb_q12_19_t)((double)(INT_MAX>>12) * f))
/** Casts a double to a vtb_q13_18_t.
 */
#define VTB_Q13_18(f) ((vtb_q13_18_t)((double)(INT_MAX>>13) * f))
/** Casts a double to a vtb_q14_17_t.
 */
#define VTB_Q14_17(f) ((vtb_q14_17_t)((double)(INT_MAX>>14) * f))
/** Casts a double to a vtb_q15_16_t.
 */
#define VTB_Q15_16(f) ((vtb_q15_16_t)((double)(INT_MAX>>15) * f))
/** Casts a double to a vtb_q16_15_t.
 */
#define VTB_Q16_15(f) ((vtb_q16_15_t)((double)(INT_MAX>>16) * f))
/** Casts a double to a vtb_q17_14_t.
 */
#define VTB_Q17_14(f) ((vtb_q17_14_t)((double)(INT_MAX>>17) * f))
/** Casts a double to a vtb_q18_13_t.
 */
#define VTB_Q18_13(f) ((vtb_q18_13_t)((double)(INT_MAX>>18) * f))
/** Casts a double to a vtb_q19_12_t.
 */
#define VTB_Q19_12(f) ((vtb_q19_12_t)((double)(INT_MAX>>19) * f))
/** Casts a double to a vtb_q20_11_t.
 */
#define VTB_Q20_11(f) ((vtb_q20_11_t)((double)(INT_MAX>>20) * f))
/** Casts a double to a vtb_q21_10_t.
 */
#define VTB_Q21_10(f) ((vtb_q21_10_t)((double)(INT_MAX>>21) * f))
/** Casts a double to a vtb_q22_9_t.
 */
#define VTB_Q22_9(f) ((vtb_q22_9_t)((double)(INT_MAX>>22) * f))
/** Casts a double to a vtb_q23_8_t.
 */
#define VTB_Q23_8(f) ((vtb_q23_8_t)((double)(INT_MAX>>23) * f))
/** Casts a double to a vtb_q24_7_t.
 */
#define VTB_Q24_7(f) ((vtb_q24_7_t)((double)(INT_MAX>>24) * f))
/** Casts a double to a vtb_q25_6_t.
 */
#define VTB_Q25_6(f) ((vtb_q25_6_t)((double)(INT_MAX>>25) * f))
/** Casts a double to a vtb_q26_5_t.
 */
#define VTB_Q26_5(f) ((vtb_q26_5_t)((double)(INT_MAX>>26) * f))
/** Casts a double to a vtb_q27_4_t.
 */
#define VTB_Q27_4(f) ((vtb_q27_4_t)((double)(INT_MAX>>27) * f))
/** Casts a double to a vtb_q28_3_t.
 */
#define VTB_Q28_3(f) ((vtb_q28_3_t)((double)(INT_MAX>>28) * f))
/** Casts a double to a vtb_q29_2_t.
 */
#define VTB_Q29_2(f) ((vtb_q29_2_t)((double)(INT_MAX>>29) * f))
/** Casts a double to a vtb_q30_1_t.
 */
#define VTB_Q30_1(f) ((vtb_q30_1_t)((double)(INT_MAX>>30) * f))
/** Casts a double to a vtb_q31_0_t.
 */
#define VTB_Q31_0(f) ((vtb_q31_0_t)((double)(INT_MAX>>31) * f))

#endif /* VTB_Q_FORMAT_H_ */
