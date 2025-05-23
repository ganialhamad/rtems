/**
 * @file
 *
 * @ingroup ppc_exc
 *
 * @brief PowerPC Exceptions implementation.
 */

/*
 * Copyright (C) 1999 Eric Valette (eric.valette@free.fr)
 *                    Canon Centre Recherche France.
 *
 * Derived from file "libcpu/powerpc/new-exceptions/bspsupport/vectors_init.c".
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#include <bsp/vectors.h>

#include <rtems/score/thread.h>
#include <rtems/score/threaddispatch.h>

#include <inttypes.h>

#ifndef __SPE__
  #define GET_GPR(gpr) (gpr)
#else
  #define GET_GPR(gpr) ((uintptr_t) ((gpr) >> 32))
#endif

/* T. Straumann: provide a stack trace
 * <strauman@slac.stanford.edu>, 6/26/2001
 */
typedef struct LRFrameRec_ {
  struct LRFrameRec_ *frameLink;
  unsigned long *lr;
} LRFrameRec, *LRFrame;

#define STACK_CLAMP 50          /* in case we have a corrupted bottom */

static uint32_t ppc_exc_get_DAR_dflt(void)
{
  uint32_t val;
  if (ppc_cpu_is_60x()) {
    PPC_SPECIAL_PURPOSE_REGISTER(PPC_DAR, val);
    return val;
  } else {
    switch (ppc_cpu_is_bookE()) {
      default:
        break;
      case PPC_BOOKE_STD:
      case PPC_BOOKE_E500:
        PPC_SPECIAL_PURPOSE_REGISTER(BOOKE_DEAR, val);
        return val;
      case PPC_BOOKE_405:
        PPC_SPECIAL_PURPOSE_REGISTER(PPC405_DEAR, val);
        return val;
    }
  }
  return 0xdeadbeef;
}

uint32_t (*ppc_exc_get_DAR)(void) = ppc_exc_get_DAR_dflt;

void BSP_printStackTrace(const BSP_Exception_frame *excPtr)
{
  LRFrame f;
  int i;
  LRFrame sp;
  void *lr;

  printk("Stack Trace: \n  ");
  if (excPtr) {
    printk("IP: 0x%08" PRIxPTR ", ", excPtr->EXC_SRR0);
    sp = (LRFrame) GET_GPR(excPtr->GPR1);
    lr = (void *) excPtr->EXC_LR;
  } else {
    /* there's no macro for this */
    __asm__ __volatile__("mr %0, 1":"=r"(sp));
    lr = (LRFrame) ppc_link_register();
  }
  printk("LR: 0x%08" PRIxPTR "\n", (uintptr_t) lr);
  for (f = (LRFrame) sp, i = 0; f->frameLink && i < STACK_CLAMP; f = f->frameLink) {
    printk("--^ 0x%08" PRIxPTR "", (uintptr_t) (f->frameLink->lr));
    if (!(++i % 5))
      printk("\n");
  }
  if (i >= STACK_CLAMP) {
    printk("Too many stack frames (stack possibly corrupted), giving up...\n");
  } else {
    if (i % 5)
      printk("\n");
  }
}

void _CPU_Exception_frame_print(const CPU_Exception_frame *excPtr)
{
  const Thread_Control *executing = _Thread_Executing;
  bool synch = (int) excPtr->_EXC_number >= 0;
  unsigned n = excPtr->_EXC_number & 0x7fff;

  printk("exception vector %d (0x%x)\n", n, n);
  printk("  next PC or address of fault = 0x%08" PRIxPTR "\n", excPtr->EXC_SRR0);
  printk("  saved MSR = 0x%08" PRIxPTR "\n", excPtr->EXC_SRR1);

  /* Try to find out more about the context where this happened */
  printk(
    "  context = %s, ISR nest level = %" PRIu32 "\n",
    _ISR_Nest_level == 0 ? "task" : "interrupt",
    _ISR_Nest_level
  );
  printk(
    "  thread dispatch disable level = %" PRIu32 "\n",
    _Thread_Dispatch_disable_level
  );

  /* Dump registers */

  printk("  R0  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR0));
  if (synch) {
    printk(" R1  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR1));
    printk(" R2  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR2));
  } else {
    printk("               ");
    printk("               ");
  }
  printk(" R3  = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR3));
  printk("  R4  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR4));
  printk(" R5  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR5));
  printk(" R6  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR6));
  printk(" R7  = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR7));
  printk("  R8  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR8));
  printk(" R9  = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR9));
  printk(" R10 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR10));
  printk(" R11 = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR11));
  printk("  R12 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR12));
  if (synch) {
    printk(" R13 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR13));
    printk(" R14 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR14));
    printk(" R15 = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR15));
    printk("  R16 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR16));
    printk(" R17 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR17));
    printk(" R18 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR18));
    printk(" R19 = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR19));
    printk("  R20 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR20));
    printk(" R21 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR21));
    printk(" R22 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR22));
    printk(" R23 = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR23));
    printk("  R24 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR24));
    printk(" R25 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR25));
    printk(" R26 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR26));
    printk(" R27 = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR27));
    printk("  R28 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR28));
    printk(" R29 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR29));
    printk(" R30 = 0x%08" PRIxPTR "", GET_GPR(excPtr->GPR30));
    printk(" R31 = 0x%08" PRIxPTR "\n", GET_GPR(excPtr->GPR31));
  } else {
    printk("\n");
  }
  printk("  CR  = 0x%08" PRIx32 "\n", excPtr->EXC_CR);
  printk("  CTR = 0x%08" PRIxPTR "\n", excPtr->EXC_CTR);
  printk("  XER = 0x%08" PRIx32 "\n", excPtr->EXC_XER);
  printk("  LR  = 0x%08" PRIxPTR "\n", excPtr->EXC_LR);

  /* Would be great to print DAR but unfortunately,
   * that is not portable across different CPUs.
   * AFAIK on classic PPC DAR is SPR 19, on the
   * 405 we have DEAR = SPR 0x3d5 and bookE says
   * DEAR = SPR 61 :-(
   */
  if (ppc_exc_get_DAR != NULL) {
    char* reg = ppc_cpu_is_60x() ? " DAR" : "DEAR";
    printk(" %s = 0x%08" PRIx32 "\n", reg, ppc_exc_get_DAR());
  }
  if (ppc_cpu_is_bookE()) {
    uint32_t esr, mcsr;
    if (ppc_cpu_is_bookE() == PPC_BOOKE_405) {
      PPC_SPECIAL_PURPOSE_REGISTER(PPC405_ESR, esr);
      PPC_SPECIAL_PURPOSE_REGISTER(PPC405_MCSR, mcsr);
    } else {
      PPC_SPECIAL_PURPOSE_REGISTER(BOOKE_ESR, esr);
      PPC_SPECIAL_PURPOSE_REGISTER(BOOKE_MCSR, mcsr);
    }
    printk("  ESR = 0x%08x\n", esr);
    printk(" MCSR = 0x%08x\n", mcsr);
  }

#ifdef PPC_MULTILIB_ALTIVEC
  {
    unsigned char *v = (unsigned char *) &excPtr->V0;
    int i;
    int j;

    printk(" VSCR = 0x%08" PRIx32 "\n", excPtr->VSCR);
    printk("VRSAVE = 0x%08" PRIx32 "\n", excPtr->VRSAVE);

    for (i = 0; i < 32; ++i) {
      printk("  V%02i = 0x", i);

      for (j = 0; j < 16; ++j) {
        printk("%02x", v[j]);
      }

      printk("\n");

      v += 16;
    }
  }
#endif

#ifdef PPC_MULTILIB_FPU
  {
    uint64_t *f = (uint64_t *) &excPtr->F0;
    int i;

    printk("FPSCR = 0x%08" PRIu64 "\n", excPtr->FPSCR);

    for (i = 0; i < 32; ++i) {
      printk("  F%02i = 0x%016" PRIu64 "\n", i, f[i]);
    }
  }
#endif

  if (executing != NULL) {
    const char *name = (const char *) &executing->Object.name;

    printk(
      "  executing thread ID = 0x%08" PRIx32 ", name = %c%c%c%c\n",
      executing->Object.id,
      name [0],
      name [1],
      name [2],
      name [3]
    );
  } else {
    printk("  executing thread pointer is NULL");
  }

  BSP_printStackTrace(excPtr);
}
