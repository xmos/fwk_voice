// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
    
#if (defined(__XS2A__) || defined(__XS3A__))

	.text
    .issue_mode  dual
	.globl	vtb_inv_X_energy_asm
	.type	vtb_inv_X_energy_asm,@function
	.cc_top vtb_inv_X_energy_asm.function,vtb_inv_X_energy_asm
	
	#define inv_X_energy_p  r0
	#define t               r1 //this is shr
	#define count           r2
	#define hi              r3
	#define lo              r11


//void vtb_inv_X_energy_asm(uint32_t * unsafe inv_X_energy, unsigned shr, unsigned count){
//    unsafe {
//        uint64_t one = ULLONG_MAX>>shr;
//        for(unsigned i=0;i<count;i++)
//            inv_X_energy[i] = one / inv_X_energy[i];
//    }
//}

#define NSTACKWORDS 0

.align 8
vtb_inv_X_energy_asm:
	dualentsp NSTACKWORDS
	{ldc hi, 32; bf count, vtb_inv_X_energy_asm_done}
	sub hi, hi, t
	{mkmsk hi, hi; sub count, count, 1}
loop:
    {ldw t, inv_X_energy_p[count]; mkmsk lo, 32}
    ldivu t, lo, hi, lo, t
    stw t, inv_X_energy_p[count]
	{bt count, loop; sub count, count, 1}
vtb_inv_X_energy_asm_done:
	retsp NSTACKWORDS

	// RETURN_REG_HOLDER
	.cc_bottom vtb_inv_X_energy_asm.function
	.set	vtb_inv_X_energy_asm.nstackwords,NSTACKWORDS
	.globl	vtb_inv_X_energy_asm.nstackwords
	.set	vtb_inv_X_energy_asm.maxcores,1
	.globl	vtb_inv_X_energy_asm.maxcores
	.set	vtb_inv_X_energy_asm.maxtimers,0
	.globl	vtb_inv_X_energy_asm.maxtimers
	.set	vtb_inv_X_energy_asm.maxchanends,0
	.globl	vtb_inv_X_energy_asm.maxchanends
.Ltmp0:
	.size	vtb_inv_X_energy_asm, .Ltmp0-vtb_inv_X_energy_asm

    .issue_mode  single
    
#endif

#if 0
/*
 * This computes inv_X_energy[i] = 1/(X_energy[i] + delta).
 * The delta limits the output size of inv_X_energy.
 */
void aec_compute_inv_X_energy(
//}
#endif
