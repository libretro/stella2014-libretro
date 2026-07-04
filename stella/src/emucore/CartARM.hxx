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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE_ARM_HXX
#define CARTRIDGE_ARM_HXX

class System;
#ifdef THUMB_SUPPORT
struct Thumbulator;
#endif

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Base class for cartridges that run ARM code through the Thumbulator
  (CDF, CDFJ and BUS). It owns the Thumbulator instance and gives the
  concrete mappers a single place to run ARM code with a 6507 cycle
  budget and to pick up the correct ARM timer rate for the console's
  TV format.

  This is a minimal C++98 base, backported from Stella 7's CartridgeARM
  but stripped to what the 2014 core needs: the ARM chip-property,
  MAM-mode, PlusROM and debugger-stats machinery of the newer class is
  not present here.
*/
class CartridgeARM : public Cartridge
{
  public:
    CartridgeARM(const Settings& settings);
    virtual ~CartridgeARM();

  protected:
    /**
      Construct the Thumbulator over the given ROM/RAM buffers. Concrete
      carts call this once their program image and RAM are laid out.

      @param rom  Pointer to the ARM ROM image (16-bit words)
      @param ram  Pointer to the ARM RAM (16-bit words)
    */
    void createThumbulator(const uint16_t* rom, uint16_t* ram);

    /**
      Select the ARM timer rate from the console's TV format. Called once
      the display format is known. Uses the exact integer ratios of ARM
      ticks per 6507 cycle (NTSC/PAL/SECAM).

      @param format  "NTSC", "PAL", "SECAM" (and their 60/50 variants)
    */
    void setArmTimingForFormat(const string& format);

    /**
      Run the ARM code with a 6507 cycle budget.

      @param cycles          6507 cycles elapsed since the previous ARM call
      @param irqDrivenAudio  Whether the ARM entry uses IRQ-driven audio
    */
    void runArm(uint32_t cycles, bool irqDrivenAudio);

    /**
      Hook for concrete carts to account for the cycles an ARM run
      consumed. The 2014 core does not feed ARM cycles back into the 6507
      clock (ARM code "runs in zero 6507 cycles"), so this is a no-op kept
      for source symmetry with the concrete mappers' call sites.
    */
    void updateCycles(int cycles);

    /**
      Serialize / restore the Thumbulator's timer state so run-ahead and
      savestates stay deterministic.
    */
    bool saveArmState(Serializer& out) const;
    bool loadArmState(Serializer& in);

  protected:
#ifdef THUMB_SUPPORT
    Thumbulator* myThumbEmulator;
#endif

  private:
    // Following constructors and assignment operators not supported
    CartridgeARM();
    CartridgeARM(const CartridgeARM&);
    CartridgeARM& operator = (const CartridgeARM&);
};

#endif
