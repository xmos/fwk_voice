// Copyright (c) 2017-2020, XMOS Ltd, All rights reserved
#ifndef _VTB_SQRT_H_
#define _VTB_SQRT_H_

#include <stdint.h>


/* For internal use */

#ifdef __XC__

uint32_t vtb_sqrt30_xs2(uint32_t); //TODO rename to _asm

{int, int} vtb_sqrt_calc_exp(const int exp, const  unsigned hr);

#endif // __XC__

#endif /* _VTB_SQRT_H_ */
