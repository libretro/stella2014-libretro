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
} Thumbulator;

/*
  Initialize a Thumbulator over caller-owned rom/ram buffers. Replaces the
  C++ constructor; no allocation is performed. 'traponfatal' is accepted
  for source compatibility but, as in the original build, unused.
*/
void thumb_init(Thumbulator* self, const uint16_t* rom, uint16_t* ram,
                int traponfatal);

/*
  Reset to the power-on state and set the entry point. Returns 0.
*/
int thumb_reset(Thumbulator* self);

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
