#include <stdio.h>
#include "ic_api.h"

ic_state_t ic_state;

static inline int32_t ashr32(int32_t x, right_shift_t shr)
{
    printf("yo");
  if(shr >= 0){
      printf("ooooooo\n");
    return x >> shr;}
  printf("ba\n");
  int64_t tmp = ((int64_t)x) << -shr;

  if(tmp > INT32_MAX)       return INT32_MAX;
  else if(tmp < INT32_MIN)  return INT32_MIN;
  else                      return tmp;
}

float_s32_t add(
    const float_s32_t x,
    const float_s32_t y)
{
  float_s32_t res;

  const headroom_t x_hr = HR_S32(x.mant);
  const headroom_t y_hr = HR_S32(y.mant);
    printf("hr1 = %d, hr2 = %d\n", x_hr, y_hr);
  const exponent_t x_min_exp = x.exp - x_hr;
  const exponent_t y_min_exp = y.exp - y_hr;
    printf("exp1 = %d, exp2 = %d\n", x_min_exp, y_min_exp);
  res.exp = MAX(x_min_exp, y_min_exp) + 1;
    printf("nex_exp = %d\n", res.exp);
  const right_shift_t x_shr = res.exp - x.exp;
  right_shift_t y_shr = res.exp - y.exp;
  y_shr = 32;
    printf("shr1 = %d, shr2 = %d\n", x_shr, y_shr);
    int32_t x_int = ashr32(x.mant, x_shr);
    int32_t y_int = ashr32(y.mant, y_shr);
    printf("%d + %d\n", x_int, y_int);
  res.mant = x_int + y_int;
    printf("\n");
  return res;
}

void test_init(void){
    ic_init(&ic_state);
    /*float_s32_t test1 = {277038593, -25};
    float_s32_t test2 = {233768088, -24};
    float_s32_t delta = {2147483646, -58};
    test1 = add(test1, delta);bfp_s32_add_scalar()
    test2 = add(test2, delta);
    printf("\ntest1 %d %d\n", test1.mant, test1.exp);
    printf("test2 %d %d\n", test2.mant, test2.exp);

    int32_t a = 0x7fffffff;
    printf("%d, %d\n", a, a>>32);*/
    //Custom setup for testing
    // ic_state.mu[0][0] = double_to_float_s32(0.0);
    ic_state.ic_adaption_controller_state.adaption_controller_config.enable_adaption_controller = 0;
    // ic_state.config_params.delta = double_to_float_s32(0.0156);

}

ic_state_t test_get_state(void){
    //bfp_s32_use_exponent(&ic_state.error_bfp[0], -31);
    //for(int v = 0; v < IC_FILTER_PHASES; v++){
        //bfp_complex_s32_use_exponent(&ic_state.H_hat_bfp[0][v], -31);
    //}
    //bfp_s32_use_exponent(&ic_state.sigma_XX_bfp[0], -31);
    //bfp_s32_use_exponent(&ic_state.X_energy_bfp[0], -31);
    //bfp_s32_use_exponent(&ic_state.inv_X_energy_bfp[0], -31);
    //bfp_complex_s32_use_exponent(&ic_state.Y_hat_bfp[0], -31);
    return ic_state;
}

void test_filter(
        int32_t y_data[IC_FRAME_ADVANCE],
        int32_t x_data[IC_FRAME_ADVANCE],
        int32_t output[IC_FRAME_ADVANCE]){
    ic_filter(&ic_state, y_data, x_data, output);
}

void test_adapt(
        uint8_t vad,
        int32_t output[IC_FRAME_ADVANCE]){
    ic_adapt(&ic_state, vad, output);
}
