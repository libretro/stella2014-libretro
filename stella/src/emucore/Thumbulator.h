/*
  Thumbulator: a small ARM7TDMI Thumb interpreter used by the DPC+
  (and later CDF/BUS) cartridge types.

  This is a C89-clean reshaping of the original C++ Thumbulator: a plain
  struct plus free functions taking an explicit self pointer, no methods,
  no references, no C++ library types. It compiles as both C++98 (as it
  ships today, called from the C++ cartridge classes) and as strict C89
  (the eventual MSVC C89 target). All declarations sit at the top of each
  block, comments are block-style only, and there is no bool/enum-class/
  new/delete/namespace usage.

  Behavior is preserved bit-for-bit from the original interpreter.
*/

#ifndef THUMBULATOR_H
#define THUMBULATOR_H

#ifdef THUMB_SUPPORT

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ARM address-space masks (16K ROM window, 8K RAM window) */
#define THUMB_ROMADDMASK 0x7FFF
#define THUMB_RAMADDMASK 0x1FFF
#define THUMB_ROMSIZE    (THUMB_ROMADDMASK + 1)
#define THUMB_RAMSIZE    (THUMB_RAMADDMASK + 1)

/* Processor modes */
#define THUMB_MODE_USR 0x10
#define THUMB_MODE_FIQ 0x11
#define THUMB_MODE_IRQ 0x12
#define THUMB_MODE_SVC 0x13
#define THUMB_MODE_ABT 0x17
#define THUMB_MODE_UND 0x1B
#define THUMB_MODE_SYS 0x1F

/* CPSR flag bits */
#define THUMB_CPSR_T (1u << 5)
#define THUMB_CPSR_F (1u << 6)
#define THUMB_CPSR_I (1u << 7)
#define THUMB_CPSR_N (1u << 31)
#define THUMB_CPSR_Z (1u << 30)
#define THUMB_CPSR_C (1u << 29)
#define THUMB_CPSR_V (1u << 28)
#define THUMB_CPSR_Q (1u << 27)

/*
  Interpreter state. Plain struct, no methods. The rom/ram buffers are
  owned by the caller (the cartridge), exactly as in the original.
*/
typedef struct Thumbulator
{
  const uint16_t* rom;   /* caller-owned ROM image (16-bit words)    */
  uint16_t*       ram;   /* caller-owned RAM (16-bit words)          */

  uint32_t halfadd;
  uint32_t cpsr;
  uint32_t reg_sys[16];  /* system mode registers      */
  uint32_t reg_svc[16];  /* supervisor mode registers  */
  uint32_t mamcr;

  /* Diagnostic counters (kept for parity with the original) */
  uint64_t instructions;
  uint64_t fetches;
  uint64_t reads;
  uint64_t writes;

  /*
    Cycle-accounting state, needed by the CDF/BUS cartridges (DPC+ does
    not use it). The ARM code paces its audio from a memory-mapped LPC2103
    Timer 1, which advances by an exact integer ratio of ARM ticks per
    6507 cycle (the 70 MHz ARM vs the ~1.19 MHz 6507). The ratio is
    carried as num/den with an integer remainder so the timer is exact
    and deterministic -- no floating point, matching the core's DPC music
    clock approach. thumb_run() (DPC+) leaves all of this at zero.
  */
  uint32_t t1_tcr;         /* Timer 1 control register (bit 0 = enable)   */
  uint32_t t1_tc;          /* Timer 1 counter (read by ARM audio code)    */
  uint32_t timer_num;      /* ARM-ticks-per-6507-cycle ratio numerator    */
  uint32_t timer_den;      /* ...denominator                              */
  uint32_t timer_frac;     /* fractional remainder, units of 1/timer_den  */

  /*
    Configurable reset vectors. DPC+ enters at 0xc0b with SP 0x40001fb4
    and LR 0xc00; the CDF/BUS drivers enter elsewhere. These are set by
    thumb_init_ex() (thumb_init keeps the DPC+ defaults), so a single
    interpreter serves every ARM cartridge.
  */
  uint32_t c_start;        /* entry PC (the +2/THUMB adjustment is applied) */
  uint32_t c_stack;        /* initial SP (r13) */
  uint32_t c_base;         /* initial LR (r14) */
} Thumbulator;

/*
  Initialize a Thumbulator over caller-owned rom/ram buffers. Replaces the
  C++ constructor; no allocation is performed. 'traponfatal' is accepted
  for source compatibility but, as in the original build, unused.
*/
void thumb_init(Thumbulator* self, const uint16_t* rom, uint16_t* ram,
                int traponfatal);

/*
  Like thumb_init, but with an explicit entry point / stack / link value,
  for cartridges whose ARM driver does not use the DPC+ layout (CDF, BUS).
  thumb_init() is equivalent to thumb_init_ex() with the DPC+ defaults
  (start 0xc09, stack 0x40001fb4, base 0xc00).
*/
void thumb_init_ex(Thumbulator* self, const uint16_t* rom, uint16_t* ram,
                   int traponfatal,
                   uint32_t c_start, uint32_t c_stack, uint32_t c_base);

/*
  Reset to the power-on state and set the entry point. Returns 0.
*/
int thumb_reset(Thumbulator* self);

/* Memory-mapped LPC2103 Timer 1 registers (used by CDF/BUS ARM audio) */
#define THUMB_T1TCR 0xE0008004u  /* Timer 1 control register */
#define THUMB_T1TC  0xE0008008u  /* Timer 1 counter          */

/*
  Set the ARM timer scaling as an exact integer ratio of ARM ticks per
  6507 cycle, selected from the console's TV format. Called by the CDF/BUS
  carts at construction / console change. Exact ratios:
    NTSC   70 / (3579545/3000000) = 42000000/715909
    PAL    70 / 1.18138           = 3500000/59069
    SECAM  70 / 1.18750           = 1120/19
*/
void thumb_set_timer_rate(Thumbulator* self, uint32_t num, uint32_t den);

/*
  Run ARM code with a 6507 cycle budget (the CDF/BUS execution model).
  Advances Timer 1 by 'cycles' 6507 cycles (scaled by the timer ratio)
  before running the ARM code to completion. 'irq_driven_audio' selects
  the ARM entry behaviour (function 254 vs 255); it is accepted for
  source parity and, as in the reference, needs no special handling here
  because the ARM code runs in zero 6507 cycles. Returns 0 on normal
  completion.
*/
int thumb_run_cycles(Thumbulator* self, uint32_t cycles, int irq_driven_audio);

/*
  Run ARM code to completion (until the driver returns to its link
  register, hits a breakpoint, or a large instruction safety cap). This is
  the original DPC+ execution model and is preserved unchanged.
*/
void thumb_run(Thumbulator* self);

#ifdef __cplusplus
}
#endif

#endif /* THUMB_SUPPORT */

#endif /* THUMBULATOR_H */
