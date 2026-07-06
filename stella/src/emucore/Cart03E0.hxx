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

#ifndef CARTRIDGE_03E0_HXX
#define CARTRIDGE_03E0_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for a Brazilian (Dynacom/CCE) 8K scheme resembling E0.
  The 4K space is four 1K slices; the low three are switchable among eight 1K
  banks and the fourth is fixed to the last 1K of the image. Slices are
  selected by accessing $x380/$x3C0 in the TIA/RIOT mirror region: bit 4 clear
  sets slice 0, bit 5 clear sets slice 1, bit 6 clear sets slice 2, each from
  address bits 0..2. The switchable slices power on at banks 4, 5 and 6.

  Reimplemented on CartridgeEnhanced. The hotspots live in the low mirror
  region and forward their underlying access, so switching happens on both
  reads and writes, as in the previous standalone version.

  @author  Thomas Jentzsch (original); 2014 port
*/
class Cartridge03E0 : public CartridgeEnhanced
{
  friend class Cartridge03E0Widget;

  public:
    Cartridge03E0(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~Cartridge03E0();

  public:
    void reset();
    void install(System& system);

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

    string name() const { return "Cartridge03E0"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x0380; }

    uint16_t calcNumSegments() const { return 4; }

  private:
    static const uint16_t BANK_SHIFT_03E0 = 10;  // 1K slices

    // The pages containing the hotspots ($0380, $03C0); accesses are
    // forwarded to the devices that originally owned them.
    System::PageAccess myHotSpotPageAccess[2];

  private:
    // Following constructors and assignment operators not supported
    Cartridge03E0();
    Cartridge03E0(const Cartridge03E0&);
    Cartridge03E0& operator=(const Cartridge03E0&);
};

#endif
