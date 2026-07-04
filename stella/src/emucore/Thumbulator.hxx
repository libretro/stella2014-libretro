//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Thumbulator.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================

#ifdef THUMB_SUPPORT

#include <sstream>
#include "bspf.hxx"

#define ROMADDMASK 0x7FFF
#define RAMADDMASK 0x1FFF

#define ROMSIZE (ROMADDMASK+1)
#define RAMSIZE (RAMADDMASK+1)

//0b10000 User       PC, R14 to R0, CPSR
//0b10001 FIQ        PC, R14_fiq to R8_fiq, R7 to R0, CPSR, SPSR_fiq
//0b10010 IRQ        PC, R14_irq, R13_irq, R12 to R0, CPSR, SPSR_irq
//0b10011 Supervisor PC, R14_svc, R13_svc, R12 to R0, CPSR, SPSR_svc
//0b10111 Abort      PC, R14_abt, R13_abt, R12 to R0, CPSR, SPSR_abt
//0b11011 Undefined  PC, R14_und, R13_und, R12 to R0, CPSR, SPSR_und
//0b11111 System

#define MODE_USR 0x10
#define MODE_FIQ 0x11
#define MODE_IRQ 0x12
#define MODE_SVC 0x13
#define MODE_ABT 0x17
#define MODE_UND 0x1B
#define MODE_SYS 0x1F

#define CPSR_T (1<<5)
#define CPSR_F (1<<6)
#define CPSR_I (1<<7)
#define CPSR_N (1<<31)
#define CPSR_Z (1<<30)
#define CPSR_C (1<<29)
#define CPSR_V (1<<28)
#define CPSR_Q (1<<27)

class Thumbulator
{
  public:
    Thumbulator(const uint16_t* rom, uint16_t* ram, bool traponfatal);
    ~Thumbulator();

    /**
      Run the ARM code, and return when finished.  A string exception is
      thrown in case of any fatal errors/aborts (if enabled), containing the
      actual error, and the contents of the registers at that point in time.
    */
    void run();

  private:
    uint32_t read_register ( uint32_t reg );
    uint32_t write_register ( uint32_t reg, uint32_t data );
    uint32_t fetch16 ( uint32_t addr );
    uint32_t fetch32 ( uint32_t addr );
    uint32_t read16 ( uint32_t addr );
    uint32_t read32 ( uint32_t );
    void write16 ( uint32_t addr, uint32_t data );
    void write32 ( uint32_t addr, uint32_t data );

    void do_zflag ( uint32_t x );
    void do_nflag ( uint32_t x );
    void do_cflag ( uint32_t a, uint32_t b, uint32_t c );
    void do_sub_vflag ( uint32_t a, uint32_t b, uint32_t c );
    void do_add_vflag ( uint32_t a, uint32_t b, uint32_t c );
    void do_cflag_bit ( uint32_t x );
    void do_vflag_bit ( uint32_t x );

    int execute ( void );
    int reset ( void );

  private:
    const uint16_t* rom;
    uint16_t* ram;

    uint32_t halfadd;
    uint32_t cpsr;
    uint32_t reg_sys[16]; //System mode
    uint32_t reg_svc[16]; //Supervisor mode
    uint32_t mamcr;

    uint64_t instructions;
    uint64_t fetches;
    uint64_t reads;
    uint64_t writes;
};

#endif
