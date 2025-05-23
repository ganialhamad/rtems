/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * RTEMS generic MPC5200 BSP
 *
 * This file contains the startup assembly code.
 *
 * References: startup code for Motorola PQII ADS board.
 * Modified for the Motorola PQII ADS board by Andy Dachs.
 * Comment for the original code:
 *
 *   I have a proprietary bootloader programmed into the flash
 *   on the board which initialises the SDRAM prior to calling
 *   this function.
 */

/*
 * Copyright (c) 2005 embedded brains GmbH & Co. KG
 * Copyright (c) 2003 IPR Engineering
 * Copyright (C) 2000 Andy Dachs <a.dachs@sstl.co.uk>
 * Copyright (C) 1995 Andrew Bray, i-cubed ltd <andy@i-cubed.co.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <rtems/asm.h>
#include <rtems/powerpc/cache.h>
#include <libcpu/powerpc-utility.h>

#include <bsp.h>
#include <bsp/mpc5200.h>

/* Some register offsets of MPC5x00 memory map registers */
.set	CS0STR,			0x04
.set	CS0STP,			0x08
.set	CS1STR,			0x0C
.set	CS1STP,			0x10
.set 	SDRAMCS0,		0x34
.set	SDRAMCS1,		0x38
.set	BOOTSTR,		0x4C
.set	BOOTSTP,		0x50
.set	ADREN,			0x54
.set	CSSR0,			0x58		/* Critical Interrupt SSR0 (603le only) */
.set	CSSR1,			0x59		/* Critical Interrupt SSR1 (603le only) */
.set    CFG,			0x20C
.set    CSBOOTROM,      	0x300
.set	CSCONTROL,		0x318
.set    CS1CONF,        0x304


/* Register offsets of MPC5x00 SDRAM memory controller registers */
.set	MOD,			0x100
.set	CTRL,			0x104
.set	CFG1,			0x108
.set	CFG2,			0x10C
.set	ADRSEL,			0x110
.set	SDELAY,			0x190

/* Register offsets of MPC5x00 GPIO registers needed */
.set	GPIOPCR,		0xb00
.set	GPIOWE,			0xc00
.set	GPIOWOD,		0xc04
.set	GPIOWDD,		0xc08
.set	GPIOWDO,		0xc0c

.set	GPIOSEN,		0xb04
.set	GPIOSDD,		0xb0c
.set	GPIOSDO,		0xb10

/* Register offsets of MPC5x00 Arbiter registers */
.set	ARBCFG,			0x1f40
.set	ARBADRTO,		0x1f58
.set	ARBDATTO,		0x1f5c
.set	ARBMPREN,		0x1f64
.set	ARBMPRIO,		0x1f68
.set	ARBSNOOP,		0x1f70

/* Some bit encodings for MGT5100 registers */
.set	ADREN_BOOT_EN,		(1 << (31 - 6))
.set	ADREN_CS0_EN,		(1 << (31 - 15))
.set	ADREN_CS1_EN,		(1 << (31 - 14))
.set	ADREN_WSE,		(1 << (31 - 31))

.set	CTRL_PRECHARGE_ALL,	(1 << (31 - 30))
.set	CTRL_REFRESH,		(1 << (31 - 29))
.set 	CTRL_MODE_EN,		(1 << (31 - 0))

.set	CSCONF_CE,		(1<<12)

/* Some fixed values for MPC5x00 registers */
.set    CSCONTROL_VAL,		0x91000000

/*
 * The DDR_MODE bit is a read-only status and should be written as 0.
 *
 * XLB_CLK = FVCO / 4
 * IPB_CLK = XLB_CLK / 2
 * PCI_CLK = IPB_CLK
 */
.set	CFG_VAL,		0x00000100

.extern boot_card

.section ".vectors", "ax"
	bl	start
	.rep 63
	.long 0x04000400
	.endr
__vec2:	b	__vec2
	.rep 63
	.long 0x04000400
	.endr
__vec3:	b	__vec3
	.rep 63
	.long 0x04000400
	.endr
__vec4:	b	__vec4
	.rep 63
	.long 0x04000400
	.endr
__vec5:	b	__vec5
	.rep 63
	.long 0x04000400
	.endr
__vec6:	b	__vec6
	.rep 63
	.long 0x04000400
	.endr
__vec7:	b	__vec7
	.rep 63
	.long 0x04000400
	.endr
__vec8:	b	__vec8
	.rep 63
	.long 0x04000400
	.endr
__vec9:	b	__vec9
	.rep 63
	.long 0x04000400
	.endr
__veca:	b	__veca
	.rep 63
	.long 0x04000400
	.endr
__vecb:	b	__vecb
	.rep 63
	.long 0x04000400
	.endr
__vecc:	b	__vecc
	.rep 63
	.long 0x04000400
	.endr
__vecd:	b	__vecd
	.rep 63
	.long 0x04000400
	.endr
__vece:	b	__vece
	.rep 63
	.long 0x04000400
	.endr
__vecf:	b	__vecf
	.rep 63+1024
	.long 0x04000400
	.endr

.section ".entry"
PUBLIC_VAR (start)
start:
/* 1st: initialization work (common for RAM/ROM startup) */
	mfmsr	r30
	SETBITS r30, r29, MSR_ME|MSR_RI
	CLRBITS r30, r29, MSR_EE
	mtmsr	r30 				/* Set RI/ME, Clr EE in MSR */

#ifdef HAS_UBOOT
	mr	r14, r3
#endif /* HAS_UBOOT */

#if defined(NEED_LOW_LEVEL_INIT)
/* initialize the MBAR (common RAM/ROM startup) */
	LWI	r31, MBAR_RESET
	LWI	r29, MBAR
	rlwinm	r30, r29,16,16,31
  	stw	r30, 0(r31) 			/* Set the MBAR */
#endif

   	LWI	r31, MBAR			/* set r31 to current MBAR */
	/* init GPIOPCR */
	lwz	r29,GPIOPCR(r31)
	LWI	r30, BSP_GPIOPCR_INITMASK
	not	r30,r30
	and	r29,r29,r30
	LWI	r30, BSP_GPIOPCR_INITVAL
	or	r29,r29,r30
	stw	r29, GPIOPCR(r31)

/* further initialization work (common RAM/ROM startup) */
	bl	TLB_init			/* Initialize TLBs */


	bl	FID_DCache			/* Flush, inhibit and disable data cache */


	bl	IDUL_ICache			/* Inhibit, disable and unlock instruction cache */


	bl	FPU_init			/* Initialize FPU */


#if defined(NEED_LOW_LEVEL_INIT)
	bl	SPRG_init			/* Initialize special purpose registers */
#endif

#if defined(NEED_LOW_LEVEL_INIT)
/* detect RAM/ROM startup (common for RAM/ROM startup) */
   	LWI     r20, bsp_rom_start		/* set the relocation offset */


	LWI     r30, CFG_VAL			/* get CFG register content */
	lwz	r30, CFG(r31)			/* set CFG register */



	lwz	r30, ADREN(r31)			/* get content of ADREN */



	TSTBITS r30, r29, ADREN_BOOT_EN
	bne	skip_ROM_start 			/* If BOOT_ROM is not enabled, skip further initialization */

/* do some board dependent configuration (unique for ROM startup) */
	LWI     r30, CSCONTROL_VAL		/* get CSCONTROL register content */
	stw	r30, CSCONTROL(r31)		/* enable internal/external bus error and master for CS */


#if defined(MPC5200_BOARD_BRS5L)
	#define CSBOOTROM_VAL 0x0101D910
#elif defined(MPC5200_BOARD_BRS6L)
	#define CSBOOTROM_VAL 0x0202D910
#endif

#ifdef CSBOOTROM_VAL
	LWI	r30, CSBOOTROM_VAL
	stw	r30, CSBOOTROM(r31)	 	/* Set CSBOOTROM */
#endif

	/* FIXME: map BOOT ROM into final location with CS0 registers */
	LWI	r30, bsp_rom_start
	rlwinm	r30, r30,17,15,31
	stw	r30, CS0STR(r31) 		/* Set CS0STR */

	LWI	r30, bsp_rom_end - 1

	rlwinm	r30, r30,17,15,31
	stw	r30, CS0STP(r31) 		/* Set CS0STP */

	lwz	r30, ADREN(r31)			/* get content of ADREN */
	SETBITS r30, r29, ADREN_CS0_EN
	stw	r30, ADREN(r31)			/* enable CS0 mapping */
	isync
	/* jump to same code in final BOOT ROM location */
	LWI	r30, reloc_in_CS0
	LWI	r29, bsp_ram_start
	sub	r30,r30,r29
	LWI	r29, bsp_rom_start
	add	r30,r30,r29
	mtctr	r30
	bctr

reloc_in_CS0:
	/* disable CSBOOT (or map it to CS0 range) */
	lwz	r30, ADREN(r31)			/* get content of ADREN */
	CLRBITS r30, r29, ADREN_BOOT_EN
	stw	r30, ADREN(r31)			/* disable BOOT mapping */

	/* init SDRAM */
	LWI	r30, bsp_ram_start
	ori	r30, r30, 0x1a			/* size code: bank is 128MByte */
	stw	r30, SDRAMCS0(r31) 		/* Set SDRAMCS0 */

	LWI	r30, bsp_ram_size
	srawi	r30, r30, 1
	ori	r30, r30, 0x1a			/* size code: bank is 128MByte */
	stw	r30, SDRAMCS1(r31) 		/* Set SDRAMCS1 */

	bl	SDRAM_init			/* Initialize SDRAM controller */

	bl	XLB_init
/* copy .text section from ROM to RAM location (unique for ROM startup) */
	LA	r30, bsp_section_text_start	/* get start address of text section in RAM */


	add	r30, r20, r30			/* get start address of text section in ROM (add reloc offset) */


	LA	r29, bsp_section_text_start	/* get start address of text section in RAM */


	LA	r28, bsp_section_text_size	/* get size of RAM image */


	bl	copy_image			/* copy text section from ROM to RAM location */


/* copy .data section from ROM to RAM location (unique for ROM startup) */
	LA	r30, bsp_section_data_start	/* get start address of data section in RAM */


	add	r30, r20, r30			/* get start address of data section in ROM (add reloc offset) */


	LA	r29, bsp_section_data_start	/* get start address of data section in RAM */


	LA	r28, bsp_section_data_size  	/* get size of RAM image */


	bl	copy_image			/* copy initialized data section from ROM to RAM location */


	LA	r29, remap_rom 			/* get compile time address of label */
	mtlr	r29

	blrl					/* now further execution RAM */

remap_rom:
/* remap BOOT ROM to CS0 (common for RAM/ROM startup) */
	lwz	r30, CSBOOTROM(r31)		/* get content of CSBOOTROM */



	CLRBITS r30, r29, CSCONF_CE
	stw	r30, CSBOOTROM(r31)		/* disable BOOT CS */



	lwz	r30, ADREN(r31)			/* get content of ADREN */



	mr	r29, r30			/* move content of r30 to r29 */


	LWI	r30, ADREN_BOOT_EN		/* mask ADREN_BOOT_EN */
	andc	r29,r29,r30


	LWI	r30, ADREN_CS0_EN		/* unmask ADREN_CS0_EN */
	or	r29,r29,r30


	stw	r29,ADREN(r31)			/* Simultaneous enable CS0 and disable BOOT address space */



	lwz	r30, CSBOOTROM(r31)		/* get content of CSBOOTROM */



	SETBITS r30, r29, CSCONF_CE
	stw	r30, CSBOOTROM(r31)		/* disable BOOT CS */



skip_ROM_start:
/* configure external DPRAM CS1 */
	LWI	r30, 0xFFFFFB10
	stw	r30, CS1CONF(r31)

/* map external DPRAM (CS1) */
	LWI	r30, bsp_dpram_start
	srawi	r30, r30, 16
	stw	r30, CS1STR(r31)

	LWI	r30, bsp_dpram_end
	srawi	r30, r30, 16
	stw	r30, CS1STP(r31)

	lwz	r30, ADREN(r31)			/* get content of ADREN */

	LWI	r29, ADREN_CS1_EN		/* unmask ADREN_CS1_EN */
	or	r30, r30,r29

	stw	r30, ADREN(r31)			/* enable CS1 */

/* clear entire on chip SRAM (unique for ROM startup) */
	lis	r30, (MBAR+ONCHIP_SRAM_OFFSET)@h	/* get start address of onchip SRAM */
	ori	r30, r30,(MBAR+ONCHIP_SRAM_OFFSET)@l
	LWI	r29, ONCHIP_SRAM_SIZE		/* get size of onchip SRAM */

	bl      clr_mem				/* Clear onchip SRAM */

#else /* defined(NEED_LOW_LEVEL_INIT) */
	bl	XLB_init
#endif /* defined(NEED_LOW_LEVEL_INIT) */
/* clear .bss section (unique for ROM startup) */
	LWI	r30, bsp_section_bss_start	/* get start address of bss section */
	LWI	r29, bsp_section_bss_size  	/* get size of bss section */


	bl      clr_mem				/* Clear the bss section */

#ifdef HAS_UBOOT
	mr	r3, r14
	bl	bsp_uboot_copy_board_info
#endif /* HAS_UBOOT */

	/*
	 * Initialize start stack (common for RAM/ROM startup).  The stacks are
	 * statically allocated and properly aligned.
	 */
	LA	r1, _ISR_Stack_area_end
	subi	r1, r1, PPC_DEFAULT_CACHE_LINE_SIZE
	li	r0, 0
	stw	r0, 0(r1)

	bl	__eabi				/* Set up EABI and SYSV environment */

/* enable dynamic power management(common for RAM/ROM startup) */
	bl	PPC_HID0_rd			/* Get the content of HID0 */

	SETBITS r30, r29, HID0_DPM
	bl	PPC_HID0_wr			/* Set DPM in HID0 */

/* clear arguments and do further init. in C (common for RAM/ROM startup) */

	/* Clear cmdline */
	xor r3, r3, r3

	bl      SYM (boot_card)			/* Call the first C routine */

twiddle:
	b	twiddle				/* We don't expect to return from boot_card but if we do */
						/* wait here for watchdog to kick us into hard reset     */

#if defined(NEED_LOW_LEVEL_INIT)
SDRAM_init:
	mflr	r12

#if defined(MPC5200_BOARD_BRS5L)
      /* set GPIO_WKUP7 pin low for 66MHz buffering */
      /* or high for 133MHz registered buffering    */
	LWI	r30, 0x80000000

	lwz	r29, GPIOWE(r31)
	or	r29,r29,r30			/* set bit 0 in r29/GPIOWE */
	stw	r29,GPIOWE(r31)

	lwz	r29, GPIOWOD(r31)
	andc	r29,r29,r30			/* clear bit 0 in r29/GPIOWOD */
	stw	r29,GPIOWOD(r31)

	lwz	r29, GPIOWDO(r31)
	andc	r29,r29,r30			/* clear bit 0 in r29/GPIOWDO */
	stw	r29,GPIOWDO(r31)

	lwz	r29, GPIOWDD(r31)
	or	r29,r29,r30			/* set bit 0 in r29/GPIOWDD */
	stw	r29,GPIOWDD(r31)

	/* activate MEM_CS1 output */
	lwz	r29, GPIOPCR(r31)
	or	r29,r29,r30			/* set bit 0 in r29/GPIOPCR */
	stw	r29,GPIOPCR(r31)

#endif

	#define SDELAY_VAL 0x00000004

#if defined(MPC5200_BOARD_BRS6L)
	#define CFG1_VAL 0x73722930
#else
	/*
	 * Single Read2Read/Write delay=0xC, Single Write2Read/Prec. delay=0x4
	 * Read CAS latency=0x2, Active2Read delay=0x2, Prec.2active delay=0x2
	 */
	#define CFG1_VAL 0xC4222600
#endif

#if defined(MPC5200_BOARD_BRS6L)
	#define CFG2_VAL 0x47770000
#else
	/* Refr.2No-Read delay=0x06, Write latency=0x0 */
	/* Burst2Read Prec.delay=0x8, Burst Write delay=0x8 */
	/* Burst Read2Write delay=0xB, Burst length=0x7, Read Tap=0x4 */
	#define CFG2_VAL 0xCCC70004
#endif

#if defined(MPC5200_BOARD_BRS5L)
	/* Mode Set enabled, Clock enabled, Auto refresh enabled, Mem. data drv */
	/* Refresh counter=0xFFFF */
	#define CTRL_VAL 0xD1470000
#elif defined(MPC5200_BOARD_BRS6L)
	#define CTRL_VAL 0xF15F0F00
#else
	/* Mode Set enabled, Clock enabled, Auto refresh enabled, Mem. data drv */
	/* Refresh counter=0xFFFF */
	#define CTRL_VAL 0xD04F0000
#endif

#if defined(MPC5200_BOARD_BRS6L)
	/* Enable DLL, normal drive strength */
	#define EMODE_VAL 0x40010000
#endif

#if defined(MPC5200_BOARD_BRS6L)
	/* Burst length = 8, burst type sequential, CAS latency 2.5, normal operation/reset DLL */
	#define MODE_VAL 0x058D0000
#else
	/* Op.Mode=0x0, Read CAS latency=0x2, Burst length=0x3, Write strobe puls */
	#define MODE_VAL 0x008D0000
#endif

#if defined(MPC5200_BOARD_BRS6L)
	/* Burst length = 8, burst type sequential, CAS latency 2.5, normal operation */
	#define SECOND_MODE_VAL (MODE_VAL & ~0x04000000)
#endif

	/* SDRAM initialization according to application note AN3221 */

	/* SDRAM controller setup */

	LWI	r3, SDELAY_VAL
	stw	r3, SDELAY(r31)

	LWI	r3, CFG1_VAL
	stw	r3, CFG1(r31)

	LWI	r3, CFG2_VAL
	stw	r3, CFG2(r31)

	LWI	r11, CTRL_VAL
	stw	r11, CTRL(r31)
	lwz	r3, CTRL(r31)

	/* Perform a PRECHARGE ALL command */
	ori	r3, r11, CTRL_PRECHARGE_ALL
	stw	r3, CTRL(r31)
	lwz	r3, CTRL(r31)

	/* Wait at least tRP time */
	li	r3, 15
	bl	ndelay

#if defined(EMODE_VAL)
	/* Write EMODE register */
	LWI	r3, EMODE_VAL
	stw	r3, MOD(r31)

	/* Wait at least tMRD time */
	li	r3, 10
	bl	ndelay
#endif

	/* Write MODE register */
	LWI	r3, MODE_VAL
	stw	r3, MOD(r31)

	/* Wait at least tMRD time */
	li	r3, 10
	bl	ndelay

	/* Perform a PRECHARGE ALL command */
	ori	r3, r11, CTRL_PRECHARGE_ALL
	stw	r3, CTRL(r31)
	lwz	r3, CTRL(r31)

	/* Wait at least tRP time */
	li	r3, 15
	bl	ndelay

	/* Perform an AUTO REFRESH */
	ori	r3, r11, CTRL_REFRESH
	stw	r3, CTRL(r31)
	lwz	r3, CTRL(r31)

	/* Wait at least tRFC time */
	li	r3, 70
	bl	ndelay

	/* Perform an AUTO REFRESH */
	ori	r3, r11, CTRL_REFRESH
	stw	r3, CTRL(r31)
	lwz	r3, CTRL(r31)

	/* Wait at least tRFC time */
	li	r3, 70
	bl	ndelay

#if defined(SECOND_MODE_VAL)
	/* Write MODE register */
	LWI	r3, SECOND_MODE_VAL
	stw	r3, MOD(r31)
#endif

	/* Disable MODE register access */
	lis	r4, CTRL_MODE_EN@h
	andc	r3, r11, r4
	stw	r3, CTRL(r31)
	lwz	r3, CTRL(r31)

	mtlr	r12
	blr

copy_image:
	mr	r27, r28
	srwi	r28, r28, 2
	mtctr	r28


	slwi	r28, r28, 2
	sub     r27, r27, r28			/* maybe some residual bytes */


copy_image_word:
	lswi	r28, r30, 0x04

	stswi   r28, r29, 0x04			/* do word copy ROM -> RAM */


	addi    r30, r30, 0x04			/* increment source pointer */
	addi    r29, r29, 0x04			/* increment destination pointer */

	bdnz	copy_image_word			/* decrement ctr and branch if not 0 */

	cmpwi	r27, 0x00			/* copy image finished ? */
	beq	copy_image_end;
	mtctr   r27				/* reload counter for residual bytes */
copy_image_byte:
	lswi	r28, r30, 0x01

	stswi   r28, r29, 0x01			/* do byte copy ROM -> RAM */


	addi    r30, r30, 0x01			/* increment source pointer */
	addi    r29, r29, 0x01			/* increment destination pointer */

	bdnz	copy_image_byte			/* decrement ctr and branch if not 0 */

copy_image_end:
	blr
#endif /* defined(NEED_LOW_LEVEL_INIT) */

FID_DCache:
	mflr	r26

	bl      PPC_HID0_rd
	TSTBITS	r30, r29, HID0_DCE
	bne     FID_DCache_exit			/* If data cache is switched of, skip further actions */

	li   	r29, PPC_D_CACHE		/* 16 Kb data cache on 603e */
	LWI	r28, bsp_section_text_start	/* Load base address (begin of RAM) */

FID_DCache_loop_1:
	lwz     r27, 0(r28)			/* Load data at address */

	addi    r28, r28, PPC_CACHE_ALIGNMENT 	/* increment cache line address */
	subi    r29, r29, PPC_CACHE_ALIGNMENT 	/* increment loop counter */
	cmpwi	r29, 0x0
	bne     FID_DCache_loop_1		/* Loop until cache size is reached */

	li   	r29, PPC_D_CACHE		/* 16 Kb data cache on 603e */
	LWI	r28, bsp_section_text_start	/* Reload base address (begin of RAM) */
	xor	r27, r27, r27
FID_DCache_loop_2:

	dcbf    r27, r28                	/* Flush and invalidate cache */

	addi    r28, r28, PPC_CACHE_ALIGNMENT 	/* increment cache line address */
	subi    r29, r29, PPC_CACHE_ALIGNMENT 	/* increment loop counter */
	cmpwi	r29, 0x0
	bne     FID_DCache_loop_2		/* Loop around until cache size is reached */

	bl      PPC_HID0_rd			/* Read HID0 */
	CLRBITS	r30, r29, HID0_DCE
	bl      PPC_HID0_wr			/* Clear DCE */

FID_DCache_exit:
	mtlr	r26
	blr

IDUL_ICache:
	mflr 	r26

	bl      PPC_HID0_rd
	TSTBITS	r30, r29, HID0_ICE
	bne     IDUL_ICache_exit		/* If instruction cache is switched of, skip further actions */

	CLRBITS r30, r29, HID0_ICE
	bl      PPC_HID0_wr 			/* Disable ICE bit */

	SETBITS r30, r29, HID0_ICFI
	bl	PPC_HID0_wr			/* Invalidate instruction cache */

	CLRBITS r30, r29, HID0_ICFI
	bl	PPC_HID0_wr			/* Disable cache invalidate */

	CLRBITS r30, r29, HID0_ILOCK
	bl      PPC_HID0_wr			/* Disable instruction cache lock */

IDUL_ICache_exit:
	mtlr	r26
	blr


TLB_init:					/* Initialize translation lookaside buffers (TLBs) */
	xor	r30, r30, r30
	xor	r29, r29, r29

TLB_init_loop:
	tlbie	r29, 0
	tlbsync
	addi	r29, r29, 0x1000
	addi	r30, r30, 0x01
	cmpli	0, 0, r30, 0x0080
	bne	TLB_init_loop
	blr

FPU_init:
	mfmsr	r30				/* get content of MSR */


	SETBITS r30, r29, MSR_FP
	mtmsr	r30				/* enable FPU and FPU exceptions */
	sync

	lfd 	f0, 0(r29)
	fmr 	f1, f0
	fmr 	f2, f0
	fmr 	f3, f0
	fmr 	f4, f0
	fmr 	f5, f0
	fmr 	f6, f0
	fmr 	f7, f0
	fmr 	f8, f0
	fmr 	f9, f0
	fmr 	f10, f0
	fmr 	f11, f0
	fmr 	f12, f0
	fmr 	f13, f0
	fmr 	f14, f0
	fmr 	f15, f0
	fmr 	f16, f0
	fmr 	f17, f0
	fmr 	f18, f0
	fmr 	f19, f0
	fmr 	f20, f0
	fmr 	f21, f0
	fmr 	f22, f0
	fmr 	f23, f0
	fmr 	f24, f0
	fmr 	f25, f0
	fmr 	f26, f0
	fmr 	f27, f0
	fmr 	f28, f0
	fmr 	f29, f0
	fmr 	f30, f0
	fmr 	f31, f0


	mtfsfi	0, 0				/* initialize bit positons in FPSCR */
	mtfsfi	1, 0
	mtfsfi	2, 0
	mtfsfi	3, 0
	mtfsfi	4, 0
	mtfsfi	5, 0
	mtfsfi	6, 0
	mtfsfi	7, 0

	blr

SPRG_init:					/* initialize registers */
	xor	r30, r30, r30

	mtspr	PPC_XER, r30
	mtspr	PPC_CTR, r30
	mtspr	DSISR, r30
	mtspr	PPC_DAR, r30
	mtspr	PPC_DEC, r30
	mtspr	SDR1, r30
	mtspr	SRR0, r30
	mtspr	SRR1, r30
	mtspr	CSSR0, r30
	mtspr	CSSR1, r30
	mtspr	SPRG0, r30
	mtspr	SPRG1, r30
	mtspr	SPRG2, r30
	mtspr	SPRG3, r30
	mtspr	SPRG4, r30
	mtspr	SPRG5, r30
	mtspr	SPRG6, r30
	mtspr	SPRG7, r30
	mtspr	PPC_EAR, r30
	mtspr	TBWU, r30
	mtspr	TBWL, r30
	mtspr	IBAT0U, r30
	mtspr	IBAT0L, r30
	mtspr	IBAT1U, r30
	mtspr	IBAT1L, r30
	mtspr	IBAT2U, r30
	mtspr	IBAT2L, r30
	mtspr	IBAT3U, r30
	mtspr	IBAT3L, r30
	mtspr	IBAT4U, r30
	mtspr	IBAT4L, r30
	mtspr	IBAT5U, r30
	mtspr	IBAT5L, r30
	mtspr	IBAT6U, r30
	mtspr	IBAT6L, r30
	mtspr	IBAT7U, r30
	mtspr	IBAT7L, r30
	mtspr	DBAT0U, r30
	mtspr	DBAT0L, r30
	mtspr	DBAT1U, r30
	mtspr	DBAT1L, r30
	mtspr	DBAT2U, r30
	mtspr	DBAT2L, r30
	mtspr	DBAT3U, r30
	mtspr	DBAT3L, r30
	mtspr	DBAT4U, r30
	mtspr	DBAT4L, r30
	mtspr	DBAT5U, r30
	mtspr	DBAT5L, r30
	mtspr	DBAT6U, r30
	mtspr	DBAT6L, r30
	mtspr	DBAT7U, r30
	mtspr	DBAT7L, r30
	mtspr	DMISS, r30
	mtspr	DCMP, r30
	mtspr	HASH1, r30
	mtspr	HASH2, r30
	mtspr	IMISS, r30
	mtspr	ICMP, r30
	mtspr	PPC_RPA, r30
	mtsr	PPC_SR0, r30
	mtsr	PPC_SR1, r30
	mtsr	PPC_SR2, r30
	mtsr	PPC_SR3, r30
	mtsr	PPC_SR4, r30
	mtsr	PPC_SR5, r30
	mtsr	PPC_SR6, r30
	mtsr	PPC_SR7, r30
	mtsr	PPC_SR8, r30
	mtsr	PPC_SR9, r30
	mtsr	PPC_SR10, r30
	mtsr	PPC_SR12, r30
	mtsr	PPC_SR13, r30
	mtsr	PPC_SR14, r30
	mtsr	PPC_SR15, r30





	blr

PPC_HID0_rd:					/* get HID0 content to r30 */


	mfspr   r30, HID0

	blr


PPC_HID0_wr:					/* put r30 content to HID0 */


	mtspr   HID0, r30

	blr

clr_mem:
	mr 	r28, r29
	srwi	r29, r29, 2
	mtctr   r29				/* set ctr reg */


	slwi	r29, r29, 2
	sub     r28, r28, r29			/* maybe some residual bytes */
	xor     r29, r29, r29


clr_mem_word:
	stswi   r29, r30, 0x04			/* store r29 (word) to r30 memory location */
	addi    r30, r30, 0x04			/* increment r30 */

	bdnz    clr_mem_word			/* dec counter and loop */


	cmpwi	r28, 0x00			/* clear mem. finished ? */
	beq	clr_mem_end;
	mtctr   r28				/* reload counter for residual bytes */
clr_mem_byte:
	stswi   r29, r30, 0x01			/* store r29 (byte) to r30 memory location  */
	addi    r30, r30, 0x01			/* update r30 */

	bdnz    clr_mem_byte			/* dec counter and loop */

clr_mem_end:
	blr					/* return */

XLB_init:
/* init arbiter and stuff... */
	LWI	r30, 0x8000a06e
	stw	r30, ARBCFG(r31) 		/* Set ARBCFG */

	LWI	r30, 0x000000ff
	stw	r30, ARBMPREN(r31) 		/* Set ARBMPREN */

	LWI	r30, 0x00001234
	stw	r30, ARBMPRIO(r31) 		/* Set ARBPRIO */

	LWI	r30, 0x0000001e
	stw	r30, ARBSNOOP(r31) 		/* Set ARBSNOOP */

	LWI	r30, 4096
	stw	r30, ARBADRTO(r31) 		/* Set ARBADRTO */
	stw	r30, ARBDATTO(r31) 		/* Set ARBDATTO */

	blr

ndelay:
	/*
	 * The maximum core frequency is 396MHz.
	 * We have (396MHz * 1024) / 10**9 == 405.
	 */
	mulli	r3, r3, 405
	srwi.	r3, r3, 10

	beqlr

	mtctr	r3

ndelay_loop:
	bdnz	ndelay_loop

	blr
