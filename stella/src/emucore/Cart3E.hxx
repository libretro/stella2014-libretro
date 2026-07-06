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
//============================================================================

#ifndef CARTRIDGE3E_HXX
#define CARTRIDGE3E_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  This is the cartridge class for Tigervision's bankswitched games with
  RAM (3F plus up to 32K of RAM). Each 2K ROM bank, or a 1K RAM bank, can
  be mapped into the lower half of the 4K address space ($F000-$F7FF); the
  upper half ($F800-$FFFF) is fixed to the last 2K of ROM.

    - poke $3F with the bank number to map that 2K ROM bank low
    - poke $3E with the bank number to map that 1K RAM bank low
      (RAM read port at $F000-$F3FF, write port at $F400-$F7FF)

  This is a thin wrapper over CartridgeEnhanced, which provides the generic
  ROM/RAM segment-banking engine; only the $3E/$3F hotspot decoding and the
  TIA chaining are specific to 3E.

  @author  Bradford W. Mott, Thomas Jentzsch (original); reimplemented on
           CartridgeEnhanced for the 2014 core
*/
class Cartridge3E : public CartridgeEnhanced
{
  friend class Cartridge3EWidget;

  public:
    Cartridge3E(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~Cartridge3E();

  protected:
    // Constructor allowing a subclass (e.g. 3E+) to specify its own
    // bankswitch size, which may differ from the raw image size.
    Cartridge3E(const uint8_t* image, uint32_t size, const Settings& settings,
                uint32_t bsSize);

  public:
    void install(System& system);

    string name() const { return "Cartridge3E"; }

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

  protected:
    // 2K ROM banks
    static const uint16_t BANK_SHIFT_3E = 11;
    // 32 x 1K RAM banks = 32K
    static const uint16_t RAM_BANKS_3E = 32;
    static const uint32_t RAM_SIZE_3E  = (uint32_t)RAM_BANKS_3E << (BANK_SHIFT_3E - 1);

  private:
    // Following constructors and assignment operators not supported
    Cartridge3E();
    Cartridge3E(const Cartridge3E&);
    Cartridge3E& operator=(const Cartridge3E&);
};

#endif
