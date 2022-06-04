// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    
#if (defined(__XS2A__) || defined(__XS3A__))

	.text
    .issue_mode  dual
	.globl	vtb_reduce_magnitude_asm
	.type	vtb_reduce_magnitude_asm,@function
	.cc_top vtb_reduce_magnitude_asm.function,vtb_reduce_magnitude_asm
	
	#define Y_p r0
	#define h r1
	#define l r2
	#define re r3
	#define s r4
	#define t r5

	#define im r11
	#define NSTACKWORDS 4

.align 8
vtb_reduce_magnitude_asm:
	dualentsp NSTACKWORDS
	std r4, r5, sp[0]
	{lsu r4, r1, r2;bf r2, vtb_reduce_magnitude_asm_div_by_zero}
	{ldc t, 0;bf r4, vtb_reduce_magnitude_asm_div_by_zero}
	ldd re, im, Y_p[0]
	ldivu s, t, h, t, l
	{ldc t, 31;shr s, s, 1}
	{ldc h, 0; ldc l, 0}
	maccs h, l, re, s
	lextract re, h, l, t, 32
	{ldc h, 0; ldc l, 0}
	maccs h, l, im, s
	lextract im, h, l, t, 32
	std re, im, Y_p[0]
	ldd r4, r5, sp[0]
	retsp NSTACKWORDS
	
vtb_reduce_magnitude_asm_div_by_zero:
	ldd r4, r5, sp[0]
	retsp NSTACKWORDS

	// RETURN_REG_HOLDER
	.cc_bottom vtb_reduce_magnitude_asm.function
	.set	vtb_reduce_magnitude_asm.nstackwords,NSTACKWORDS
	.globl	vtb_reduce_magnitude_asm.nstackwords
	.set	vtb_reduce_magnitude_asm.maxcores,1
	.globl	vtb_reduce_magnitude_asm.maxcores
	.set	vtb_reduce_magnitude_asm.maxtimers,0
	.globl	vtb_reduce_magnitude_asm.maxtimers
	.set	vtb_reduce_magnitude_asm.maxchanends,0
	.globl	vtb_reduce_magnitude_asm.maxchanends
.Ltmp0:
	.size	vtb_reduce_magnitude_asm, .Ltmp0-vtb_reduce_magnitude_asm

    .issue_mode  single
    
#endif
