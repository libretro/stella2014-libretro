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

#ifndef CARTRIDGEE0_HXX
#define CARTRIDGEE0_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Parker Brothers' 8K games. The 4K address space is
  divided into four 1K slices; the ROM image is eight 1K segments. Slices 0, 1
  and 2 can each independently select any of the eight segments, while slice 3
  is fixed to the last segment (segment 7):

    - $1FE0-$1FE7 select the segment for slice 0 ($1000-$13FF)
    - $1FE8-$1FEF select the segment for slice 1 ($1400-$17FF)
    - $1FF0-$1FF7 select the segment for slice 2 ($1800-$1BFF)

  Reimplemented as a thin CartridgeEnhanced subclass: the base class provides
  the per-segment banking and page installation; E0 only sets the 1K segment
  geometry, decodes its hotspots, and seeds the power-on slices.

  @author  Bradford W. Mott
*/
class CartridgeE0 : public CartridgeEnhanced
{
  friend class CartridgeE0Widget;

  public:
    CartridgeE0(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeE0();

  public:
    void reset();

    string name() const { return "CartridgeE0"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x1FE0; }

  private:
    // 1K ROM segments (four slices in the 4K space)
    static const uint16_t BANK_SHIFT_E0 = 10;

  private:
    // Following constructors and assignment operators not supported
    CartridgeE0();
    CartridgeE0(const CartridgeE0&);
    CartridgeE0& operator=(const CartridgeE0&);
};

#endif
