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

#ifndef CARTRIDGEF4_HXX
#define CARTRIDGEF4_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Atari's 32K bankswitched games. There are eight 4K
  banks, selected by accessing hotspots $1FF4 (bank 0) through $1FFB (bank 7).

  Reimplemented as a thin CartridgeEnhanced subclass; F4 only has to describe
  its eight hotspots.

  @author  Bradford W. Mott
*/
class CartridgeF4 : public CartridgeEnhanced
{
  friend class CartridgeF4Widget;

  public:
    CartridgeF4(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeF4();

  public:
    string name() const { return "CartridgeF4"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x1FF4; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeF4();
    CartridgeF4(const CartridgeF4&);
    CartridgeF4& operator=(const CartridgeF4&);
};

#endif
