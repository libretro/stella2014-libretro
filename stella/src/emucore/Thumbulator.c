/*
  Thumbulator implementation (C89-clean reshaping of the original C++
  interpreter). See Thumbulator.h for the design notes. Behavior is
  preserved bit-for-bit; only the surface form changed (struct + free
  functions, self pointer, declarations at top of block, block comments).
*/

#ifdef THUMB_SUPPORT

#include "Thumbulator.h"

/* Forward declarations of the internal helpers (static, one TU). */
static uint32_t thumb_fetch16(Thumbulator* self, uint32_t addr);
static uint32_t thumb_fetch32(Thumbulator* self, uint32_t addr);
static void     thumb_write16(Thumbulator* self, uint32_t addr, uint32_t data);
static void     thumb_write32(Thumbulator* self, uint32_t addr, uint32_t data);
static uint32_t thumb_read16(Thumbulator* self, uint32_t addr);
static uint32_t thumb_read32(Thumbulator* self, uint32_t addr);
static uint32_t thumb_read_register(Thumbulator* self, uint32_t reg);
static uint32_t thumb_write_register(Thumbulator* self, uint32_t reg,
                                     uint32_t data);
static void thumb_do_zflag(Thumbulator* self, uint32_t x);
static void thumb_do_nflag(Thumbulator* self, uint32_t x);
static void thumb_do_cflag(Thumbulator* self, uint32_t a, uint32_t b,
                           uint32_t c);
static void thumb_do_sub_vflag(Thumbulator* self, uint32_t a, uint32_t b,
                               uint32_t c);
static void thumb_do_add_vflag(Thumbulator* self, uint32_t a, uint32_t b,
                               uint32_t c);
static void thumb_do_cflag_bit(Thumbulator* self, uint32_t x);
static void thumb_do_vflag_bit(Thumbulator* self, uint32_t x);
static int  thumb_execute(Thumbulator* self);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void thumb_init_ex(Thumbulator* self, const uint16_t* rom, uint16_t* ram,
                   int traponfatal,
                   uint32_t c_start, uint32_t c_stack, uint32_t c_base)
{
  int i;

  /* Reference the (upstream-dead) fetch32 helper so it is retained for
     parity without tripping unused-function diagnostics. */
  (void)traponfatal;
  (void)thumb_fetch32;
  self->rom = rom;
  self->ram = ram;
  for(i = 0; i < 16; ++i)
  {
    self->reg_sys[i] = 0;
    self->reg_svc[i] = 0;
  }
  self->halfadd = 0;
  self->cpsr = 0;
  self->mamcr = 0;
  self->instructions = 0;
  self->fetches = 0;
  self->reads = 0;
  self->writes = 0;

  self->t1_tcr = 0;
  self->t1_tc = 0;
  self->timer_num = 42000000u;  /* default to NTSC until the cart sets it */
  self->timer_den = 715909u;
  self->timer_frac = 0;

  self->c_start = c_start;
  self->c_stack = c_stack;
  self->c_base = c_base;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void thumb_init(Thumbulator* self, const uint16_t* rom, uint16_t* ram,
                int traponfatal)
{
  /* DPC+ layout: entry 0xc09 (reset applies +2 -> 0xc0b), SP 0x40001fb4,
     LR 0xc00. */
  thumb_init_ex(self, rom, ram, traponfatal,
                0x00000c09u, 0x40001fb4u, 0x00000c00u);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static uint32_t thumb_fetch16(Thumbulator* self, uint32_t addr)
{
  uint32_t data;

  self->fetches++;

  switch(addr & 0xF0000000)
  {
    case 0x00000000: /* ROM */
      addr &= THUMB_ROMADDMASK;
      if(addr < 0x50)
        return 0;
      addr >>= 1;
#ifdef MSB_FIRST
      data = ((self->rom[addr] >> 8) | (self->rom[addr] << 8)) & 0xffff;
#else
      data = self->rom[addr];
#endif
      return data;

    case 0x40000000: /* RAM */
      addr &= THUMB_RAMADDMASK;
      addr >>= 1;
#ifdef MSB_FIRST
      data = ((self->ram[addr] >> 8) | (self->ram[addr] << 8)) & 0xffff;
#else
      data = self->ram[addr];
#endif
      return data;

    default:
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static uint32_t thumb_fetch32(Thumbulator* self, uint32_t addr)
{
  uint32_t data;

  switch(addr & 0xF0000000)
  {
    case 0x00000000: /* ROM */
      if(addr < 0x50)
      {
        data = thumb_read32(self, addr);
        if(addr == 0x00000000) return data;
        if(addr == 0x00000004) return data;
        return 0;
      }
      /* fall through */

    case 0x40000000: /* RAM */
      data  = thumb_fetch16(self, addr + 2);
      data <<= 16;
      data |= thumb_fetch16(self, addr + 0);
      return data;

    default:
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_write16(Thumbulator* self, uint32_t addr, uint32_t data)
{
  if((addr > 0x40001fff) && (addr < 0x50000000))
    return;
  else if((addr > 0x40000028) && (addr < 0x40000c00))
    return;
  if(addr & 1)
    return;

  self->writes++;

  switch(addr & 0xF0000000)
  {
    case 0x40000000: /* RAM */
      addr &= THUMB_RAMADDMASK;
      addr >>= 1;
#ifdef MSB_FIRST
      self->ram[addr] = (((data & 0xFFFF) >> 8) |
                         ((data & 0xffff) << 8)) & 0xffff;
#else
      self->ram[addr] = data & 0xFFFF;
#endif
      return;

    case 0xE0000000: /* MAMCR */
      if(addr == 0xE01FC000)
      {
        self->mamcr = data;
        return;
      }
      break;

    default:
      break;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_write32(Thumbulator* self, uint32_t addr, uint32_t data)
{
  if(addr & 3)
    return;

  switch(addr & 0xF0000000)
  {
    case 0xF0000000: /* halt */
      return;

    case 0xE0000000: /* periph */
      switch(addr)
      {
        case THUMB_T1TCR: /* Timer 1 control register */
          self->t1_tcr = data;
          break;
        case THUMB_T1TC:  /* Timer 1 counter */
          self->t1_tc = data;
          break;
        default:
          break;
      }
      return;

    case 0xD0000000: /* debug */
      return;

    case 0x40000000: /* RAM */
      thumb_write16(self, addr + 0, (data >>  0) & 0xFFFF);
      thumb_write16(self, addr + 2, (data >> 16) & 0xFFFF);
      return;

    default:
      break;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static uint32_t thumb_read16(Thumbulator* self, uint32_t addr)
{
  uint32_t data;

  if((addr > 0x40001fff) && (addr < 0x50000000))
    return 0;
  else if((addr > 0x7fff) && (addr < 0x10000000))
    return 0;
  if(addr & 1)
    return 0;

  self->reads++;

  switch(addr & 0xF0000000)
  {
    case 0x00000000: /* ROM */
      addr &= THUMB_ROMADDMASK;
      addr >>= 1;
#ifdef MSB_FIRST
      data = ((self->rom[addr] >> 8) | (self->rom[addr] << 8)) & 0xffff;
#else
      data = self->rom[addr];
#endif
      return data;

    case 0x40000000: /* RAM */
      addr &= THUMB_RAMADDMASK;
      addr >>= 1;
#ifdef MSB_FIRST
      data = ((self->ram[addr] >> 8) | (self->ram[addr] << 8)) & 0xffff;
#else
      data = self->ram[addr];
#endif
      return data;

    case 0xE0000000: /* MAMCR */
      if(addr == 0xE01FC000)
        return self->mamcr;
      break;

    default:
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static uint32_t thumb_read32(Thumbulator* self, uint32_t addr)
{
  uint32_t data;

  if(addr & 3)
    return 0;

  switch(addr & 0xF0000000)
  {
    case 0x00000000: /* ROM */
    case 0x40000000: /* RAM */
      data  = thumb_read16(self, addr + 2);
      data <<= 16;
      data |= thumb_read16(self, addr + 0);
      return data;

    case 0xE0000000: /* periph */
      if(addr == THUMB_T1TC)      /* ARM audio code reads the timer */
        return self->t1_tc;
      else if(addr == THUMB_T1TCR)
        return self->t1_tcr;
      break;

    default:
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static uint32_t thumb_read_register(Thumbulator* self, uint32_t reg)
{
  uint32_t data;

  data = 0;
  reg &= 0xF;
  switch(self->cpsr & 0x1F)
  {
    case THUMB_MODE_SVC:
      switch(reg)
      {
        default:          data = self->reg_sys[reg]; break;
        case 13: case 14: data = self->reg_svc[reg]; break;
      }
      return data;

    default:
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static uint32_t thumb_write_register(Thumbulator* self, uint32_t reg,
                                     uint32_t data)
{
  reg &= 0xF;
  switch(self->cpsr & 0x1F)
  {
    case THUMB_MODE_SVC:
      switch(reg)
      {
        default:          self->reg_sys[reg] = data; break;
        case 13: case 14: self->reg_svc[reg] = data; break;
      }
      return data;

    default:
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_zflag(Thumbulator* self, uint32_t x)
{
  if(x == 0) self->cpsr |=  THUMB_CPSR_Z;
  else       self->cpsr &= ~THUMB_CPSR_Z;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_nflag(Thumbulator* self, uint32_t x)
{
  if(x & 0x80000000) self->cpsr |=  THUMB_CPSR_N;
  else               self->cpsr &= ~THUMB_CPSR_N;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_cflag(Thumbulator* self, uint32_t a, uint32_t b,
                           uint32_t c)
{
  uint32_t rc;

  self->cpsr &= ~THUMB_CPSR_C;
  rc = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c;   /* carry in  */
  rc = (rc >> 31) + (a >> 31) + (b >> 31);        /* carry out */
  if(rc & 2)
    self->cpsr |= THUMB_CPSR_C;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_sub_vflag(Thumbulator* self, uint32_t a, uint32_t b,
                               uint32_t c)
{
  self->cpsr &= ~THUMB_CPSR_V;

  /* if the sign bits are different */
  if((a & 0x80000000) ^ (b & 0x80000000))
  {
    /* and result matches b */
    if((b & 0x80000000) == (c & 0x80000000))
      self->cpsr |= THUMB_CPSR_V;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_add_vflag(Thumbulator* self, uint32_t a, uint32_t b,
                               uint32_t c)
{
  self->cpsr &= ~THUMB_CPSR_V;

  /* if sign bits are the same */
  if((a & 0x80000000) == (b & 0x80000000))
  {
    /* and the result is different */
    if((b & 0x80000000) != (c & 0x80000000))
      self->cpsr |= THUMB_CPSR_V;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_cflag_bit(Thumbulator* self, uint32_t x)
{
  if(x) self->cpsr |=  THUMB_CPSR_C;
  else  self->cpsr &= ~THUMB_CPSR_C;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_do_vflag_bit(Thumbulator* self, uint32_t x)
{
  if(x) self->cpsr |=  THUMB_CPSR_V;
  else  self->cpsr &= ~THUMB_CPSR_V;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int thumb_reset(Thumbulator* self)
{
  self->cpsr = THUMB_CPSR_T | THUMB_CPSR_I | THUMB_CPSR_F | THUMB_MODE_SVC;

  self->reg_svc[13] = self->c_stack;         /* sp                        */
  self->reg_svc[14] = self->c_base;          /* lr                        */
  self->reg_sys[15] = self->c_start + 2;     /* entry point (+2 pipeline) */
  self->mamcr = 0;

  self->instructions = 0;
  self->fetches = 0;
  self->reads = 0;
  self->writes = 0;

  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void thumb_set_timer_rate(Thumbulator* self, uint32_t num, uint32_t den)
{
  if(den != 0)
  {
    self->timer_num = num;
    self->timer_den = den;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* The 64-bit division below is kept in its own function with optimization
   disabled under MSVC. The MSVC 2005 optimizer aborts with an internal
   compiler error (C1001) during instruction selection for an inlined
   uint64_t divide (the same failure fixed earlier in the TIA paddle path);
   isolating it here avoids that while leaving every other function fully
   optimized and every other compiler unaffected. */
#if defined(_MSC_VER)
  #pragma optimize("", off)
#endif
static uint32_t thumb_timer_ticks(uint32_t cycles, uint32_t num,
                                  uint32_t den, uint32_t* frac)
{
  uint64_t acc;
  uint32_t ticks;

  acc = (uint64_t)cycles * num + *frac;
  ticks = (uint32_t)(acc / den);
  *frac = (uint32_t)(acc % den);
  return ticks;
}
#if defined(_MSC_VER)
  #pragma optimize("", on)
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void thumb_update_timer(Thumbulator* self, uint32_t cycles)
{
  /* Timer 1 only advances while enabled (control bit 0). Advance it by
     'cycles' 6507 cycles scaled by the exact ARM-ticks-per-6507-cycle
     ratio, carrying the fractional remainder so the timer is exact and
     bit-for-bit deterministic across platforms. */
  if(self->t1_tcr & 1)
    self->t1_tc += thumb_timer_ticks(cycles, self->timer_num,
                                     self->timer_den, &self->timer_frac);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int thumb_run_cycles(Thumbulator* self, uint32_t cycles, int irq_driven_audio)
{
  (void)irq_driven_audio;  /* ARM code runs in zero 6507 cycles; no special
                              handling needed here, as in the reference. */
  thumb_update_timer(self, cycles);

  thumb_reset(self);
  for(;;)
  {
    if(thumb_execute(self))
      break;
    if(self->instructions > 500000) /* safety cap, as in the original */
      break;
  }
  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void thumb_run(Thumbulator* self)
{
  thumb_reset(self);
  for(;;)
  {
    if(thumb_execute(self))
      break;
    if(self->instructions > 500000) /* safety cap, as in the original */
      break;
  }
}

/* The large instruction interpreter body follows, included to keep this
   file readable; it is still part of this single translation unit. It
   references thumb_fetch32 in its fatal-branch path, so that helper is
   not dead. */
#include "Thumbulator_execute.ci"

#endif /* THUMB_SUPPORT */
