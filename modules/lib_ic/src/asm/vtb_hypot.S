// Copyright 2016-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

	.section    .cp.rodata, "ac", @progbits
	g_hyp_scalar:
	.word 0x9b74eda7 // (unsigned)((double)UINT_MAX*0.607252935008881);

	.section	.dp.data,"awd",@progbits
	.text
	.cc_top vtb_hypot_asm.function
	.globl	vtb_hypot_asm
	.align	4
	.type	vtb_hypot_asm,@function

#define ITT(N) \
	{clz r3, r0; add r11, r11, 1};\
	{bf r3, neg_ ## N; shr r3, r1, r11};\
		{shr r2, r0, r11;sub r0, r0, r3};\
		{bu next_ ## N; add r1, r1, r2};\
	neg_ ## N:;\
		ashr r2, r0, r11;\
		{add r0, r0, r3; sub r1, r1, r2};\
	next_ ## N:

#define ITT_FINAL(N) \
	{clz r3, r0; add r11, r11, 1};\
	{bf r3, neg_ ## N;};\
		{shr r2, r0, r11; ldc r3, 0};\
		{bu next_ ## N; add r1, r1, r2};\
	neg_ ## N:;\
		ashr r2, r0, r11;\
		{sub r1, r1, r2; ldc r3, 0};\
	next_ ## N:

#define MAX_ITTERATIONS 24

vtb_hypot_asm:
.align 8
.issue_mode dual
    {dualentsp 0;nop}
	ashr r0, r0, 2
	{clz r2, r1; mov r11, r2}
	{ldc r3, MAX_ITTERATIONS; nop }
	{sub r11, r3, r11; ldc r3, 6}
	mul r11, r11, r3
	{bt r2, start; clz r2, r0}
		{neg r1, r1; nop}
	start:
	{bt r2, neg_0;shr r1, r1, 2}
		{add r0, r0, r1; sub r1, r1, r0}
		{bu next_0; nop}
	neg_0:
		{sub r0, r0, r1; add r1, r1, r0}
	next_0:
	{mov r2, r11; ldc r11, 0}
	{bru r2; nop}
	ITT(1)
	ITT(2)
	ITT(3)
	ITT(4)
	ITT(5)
	ITT(6)
	ITT(7)
	ITT(8)
	ITT(9)
	ITT(10)
	ITT(11)
	ITT(12)
	ITT(13)
	ITT(14)
	ITT(15)
	ITT(16)
	ITT(17)
	ITT(18)
	ITT(19)
	ITT(20)
	ITT(21)
	ITT(22)
	ITT(23)
	ITT_FINAL(24)

	ldw r0, cp[g_hyp_scalar]
	maccu r2, r3, r1, r0
	{retsp 0;shl r0, r2, 2}


.tmp_vtb_hypot_asm:
	.size	vtb_hypot_asm, .tmp_vtb_hypot_asm-vtb_hypot_asm
	.align	4
	.cc_bottom vtb_hypot_asm.function

	.set	vtb_hypot_asm.nstackwords,0
	.globl	vtb_hypot_asm.nstackwords
	.set	vtb_hypot_asm.maxcores,1
	.globl	vtb_hypot_asm.maxcores
	.set	vtb_hypot_asm.maxtimers,0
	.globl	vtb_hypot_asm.maxtimers
	.set	vtb_hypot_asm.maxchanends,0
	.globl	vtb_hypot_asm.maxchanends

	.cc_top vtb_scale_asm.function
	.globl	vtb_scale_asm
	.align	4
	.type	vtb_scale_asm,@function
vtb_scale_asm:
.align 8
.issue_mode dual
	ldc r3, 0
	ldivu r1,r11,r3,r1,r2
	ldivu r0,r2,r11,r0,r2
	retsp 0

.tmp_vtb_scale_asm:
	.size	vtb_scale_asm, .tmp_vtb_scale_asm-vtb_scale_asm
	.align	4
	.cc_bottom vtb_scale_asm.function

	.set	vtb_scale_asm.nstackwords,0
	.globl	vtb_scale_asm.nstackwords
	.set	vtb_scale_asm.maxcores,1
	.globl	vtb_scale_asm.maxcores
	.set	vtb_scale_asm.maxtimers,0
	.globl	vtb_scale_asm.maxtimers
	.set	vtb_scale_asm.maxchanends,0
	.globl	vtb_scale_asm.maxchanends



