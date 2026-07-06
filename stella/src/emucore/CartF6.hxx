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

#ifndef CARTRIDGEF6_HXX
#define CARTRIDGEF6_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Atari's 16K bankswitched games. There are four 4K
  banks, selected by accessing hotspots $1FF6 (bank 0) through $1FF9 (bank 3).

  Reimplemented as a thin CartridgeEnhanced subclass; F6 only has to describe
  its four hotspots.

  @author  Bradford W. Mott
*/
class CartridgeF6 : public CartridgeEnhanced
{
  friend class CartridgeF6Widget;

  public:
    CartridgeF6(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeF6();

  public:
    string name() const { return "CartridgeF6"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x1FF6; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeF6();
    CartridgeF6(const CartridgeF6&);
    CartridgeF6& operator=(const CartridgeF6&);
};

#endif
