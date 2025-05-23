/*
 * RTEMS generic MPC5200 BSP
 *
 * This file contains the code to initialize the cpu.
 */

/*
 * Copyright (C) 2003 IPR Engineering
 * Copyright (C) 2005 embedded brains GmbH & Co. KG
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

#include <stdbool.h>
#include <string.h>

#include <libcpu/powerpc-utility.h>
#include <libcpu/mmu.h>

#include <bsp.h>
#include <bsp/mpc5200.h>

#define SET_DBAT( n, uv, lv) \
  do { \
    PPC_SET_SPECIAL_PURPOSE_REGISTER( DBAT##n##L, lv); \
    PPC_SET_SPECIAL_PURPOSE_REGISTER( DBAT##n##U, uv); \
  } while (0)

static void calc_dbat_regvals(
  BAT *bat_ptr,
  uint32_t base_addr,
  uint32_t size,
  bool flg_w,
  bool flg_i,
  bool flg_m,
  bool flg_g,
  uint32_t flg_bpp
)
{
  uint32_t block_mask = 0xffffffff;
  uint32_t end_addr = base_addr + size - 1;

  /* Determine block mask, that overlaps the whole block */
  while ((end_addr & block_mask) != (base_addr & block_mask)) {
    block_mask <<= 1;
  }

  bat_ptr->batu.bepi = base_addr >> (32 - 15);
  bat_ptr->batu.bl   = ~(block_mask >> (28 - 11));
  bat_ptr->batu.vs   = 1;
  bat_ptr->batu.vp   = 1;

  bat_ptr->batl.brpn = base_addr  >> (32 - 15);
  bat_ptr->batl.w    = flg_w;
  bat_ptr->batl.i    = flg_i;
  bat_ptr->batl.m    = flg_m;
  bat_ptr->batl.g    = flg_g;
  bat_ptr->batl.pp   = flg_bpp;
}

static inline void enable_bat_4_to_7(void)
{
  PPC_SET_SPECIAL_PURPOSE_REGISTER_BITS(HID2, BSP_BBIT32(13));
}

static void cpu_init_bsp(void)
{
  BAT dbat;

#if defined(MPC5200_BOARD_BRS5L) || defined(MPC5200_BOARD_BRS6L)
  calc_dbat_regvals(
    &dbat,
    (uint32_t) bsp_ram_start,
    (uint32_t) bsp_ram_size,
    false,
    false,
    false,
    false,
    BPP_RW
  );
  SET_DBAT(0,dbat.batu,dbat.batl);

  calc_dbat_regvals(
    &dbat,
    (uint32_t) bsp_rom_start,
    (uint32_t) bsp_rom_size,
    false,
    false,
    false,
    false,
    BPP_RX
  );
  SET_DBAT(1,dbat.batu,dbat.batl);

  calc_dbat_regvals(
    &dbat,
    (uint32_t) MBAR,
    128 * 1024,
    false,
    true,
    false,
    true,
    BPP_RW
  );
  SET_DBAT(2,dbat.batu,dbat.batl);
#elif defined (HAS_UBOOT)
  uint32_t start = 0;

  /*
   * Accesses (also speculative accesses) outside of the RAM area are a
   * disaster especially in combination with the BestComm.  For safety reasons
   * we make the available RAM a little bit smaller to have an unused area at
   * the end.
   */
  bsp_uboot_board_info.bi_memsize -= 4 * 1024;

  /*
   * Program BAT0 for RAM
   */
  calc_dbat_regvals(
    &dbat,
    bsp_uboot_board_info.bi_memstart,
    bsp_uboot_board_info.bi_memsize,
    false,
    false,
    false,
    false,
    BPP_RW
  );
  SET_DBAT(0,dbat.batu,dbat.batl);

  /*
   * Program BAT1 for Flash
   *
   * WARNING!! Some Freescale LITE5200B boards ship with a version of
   * U-Boot that lies about the starting address of Flash.  This check
   * corrects that.
   */
  if ((bsp_uboot_board_info.bi_flashstart + bsp_uboot_board_info.bi_flashsize)
    < bsp_uboot_board_info.bi_flashstart) {
    start = 0 - bsp_uboot_board_info.bi_flashsize;
  } else {
    start = bsp_uboot_board_info.bi_flashstart;
  }
  calc_dbat_regvals(
    &dbat,
    start,
    bsp_uboot_board_info.bi_flashsize,
    false,
    false,
    false,
    false,
    BPP_RX
  );
  SET_DBAT(1,dbat.batu,dbat.batl);

  /*
   * Program BAT2 for the MBAR
   */
  calc_dbat_regvals(
    &dbat,
    (uint32_t) MBAR,
    128 * 1024,
    false,
    true,
    false,
    true,
    BPP_RW
  );
  SET_DBAT(2,dbat.batu,dbat.batl);

  /*
   * If there is SRAM, program BAT3 for that memory
   */
  if (bsp_uboot_board_info.bi_sramsize != 0) {
    calc_dbat_regvals(
      &dbat,
      bsp_uboot_board_info.bi_sramstart,
      bsp_uboot_board_info.bi_sramsize,
      false,
      true,
      true,
      true,
      BPP_RW
    );
    SET_DBAT(3,dbat.batu,dbat.batl);
  }
#else
#warning "Using BAT register values set by environment"
#endif

#if defined(MPC5200_BOARD_DP2)
  enable_bat_4_to_7();

  /* FPGA */
  calc_dbat_regvals(
    &dbat,
    0xf0020000,
    128 * 1024,
    false,
    true,
    false,
    true,
    BPP_RW
  );
  SET_DBAT(4, dbat.batu, dbat.batl);
#elif defined(MPC5200_BOARD_PM520_ZE30)
  enable_bat_4_to_7();

  /* External CC770 CAN controller available in version 2 */
  calc_dbat_regvals(
    &dbat,
    0xf2000000,
    128 * 1024,
    false,
    true,
    false,
    true,
    BPP_RW
  );
  SET_DBAT(4, dbat.batu, dbat.batl);
#elif defined(MPC5200_BOARD_BRS5L)
  calc_dbat_regvals(
    &dbat,
    (uint32_t) bsp_dpram_start,
    128 * 1024,
    false,
    true,
    false,
    true,
    BPP_RW
  );
  SET_DBAT(3,dbat.batu,dbat.batl);
#elif defined(MPC5200_BOARD_BRS6L)
  enable_bat_4_to_7();

  /* FPGA */
  calc_dbat_regvals(
    &dbat,
    MPC5200_BRS6L_FPGA_BEGIN,
    MPC5200_BRS6L_FPGA_SIZE,
    false,
    true,
    false,
    true,
    BPP_RW
  );
  SET_DBAT(3,dbat.batu,dbat.batl);

  /* MRAM */
  calc_dbat_regvals(
    &dbat,
    MPC5200_BRS6L_MRAM_BEGIN,
    MPC5200_BRS6L_MRAM_SIZE,
    true,
    false,
    false,
    false,
    BPP_RW
  );
  SET_DBAT(4,dbat.batu,dbat.batl);
#endif
}

void cpu_init(void)
{
  uint32_t msr;

  #if BSP_INSTRUCTION_CACHE_ENABLED
    rtems_cache_enable_instruction();
  #endif

  /* Set up DBAT registers in MMU */
  cpu_init_bsp();

  #if defined(SHOW_MORE_INIT_SETTINGS)
    { extern void ShowBATS(void);
      ShowBATS();
    }
  #endif

  /* Read MSR */
  msr = ppc_machine_state_register();

  /* Enable data MMU in MSR */
  msr |= MSR_DR;

  /* Update MSR */
  ppc_set_machine_state_register( msr);

  #if BSP_DATA_CACHE_ENABLED
    rtems_cache_enable_data();
  #endif
}
