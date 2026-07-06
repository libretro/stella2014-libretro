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

#ifndef CARTRIDGE3EPLUS_HXX
#define CARTRIDGE3EPLUS_HXX

#include "bspf.hxx"
#include "Cart3E.hxx"

/**
  Cartridge class for the "3E+" scheme, an enhanced 3E by Thomas Jentzsch.

  The 4K address space is split into four 1K segments, and (unlike plain 3E,
  which only ever maps the low segment) any 1K ROM bank or 512-byte RAM bank
  can be mapped into any of the four segments:

    - poke $3F: low 6 bits = ROM bank, top 2 bits = target segment
    - poke $3E: low 6 bits = RAM bank, top 2 bits = target segment

  Up to 64 RAM banks (32K) are supported. Detection is by the key 'TJ3E'.

  @author  Thomas Jentzsch (original); reimplemented on CartridgeEnhanced
           for the 2014 core
*/
class Cartridge3EPlus : public Cartridge3E
{
  friend class Cartridge3EPlusWidget;

  public:
    Cartridge3EPlus(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~Cartridge3EPlus();

  public:
    void reset();

    string name() const { return "Cartridge3E+"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    // The 3E+ space is always four 1K segments
    uint16_t calcNumSegments() const { return 4; }

  private:
    // Following constructors and assignment operators not supported
    Cartridge3EPlus();
    Cartridge3EPlus(const Cartridge3EPlus&);
    Cartridge3EPlus& operator=(const Cartridge3EPlus&);

  private:
    // 1K ROM segments
    static const uint16_t BANK_SHIFT_3EP = 10;
    // 64 x 512-byte RAM banks = 32K
    static const uint16_t RAM_BANKS_3EP = 64;
    static const uint32_t RAM_SIZE_3EP  = (uint32_t)RAM_BANKS_3EP << (BANK_SHIFT_3EP - 1);
};

#endif
