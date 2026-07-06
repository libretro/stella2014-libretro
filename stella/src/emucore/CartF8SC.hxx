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

#ifndef CARTRIDGEF8SC_HXX
#define CARTRIDGEF8SC_HXX

#include "bspf.hxx"
#include "CartF8.hxx"

/**
  Cartridge class used for Atari's 8K bankswitched games with 128 bytes of
  RAM (the "superchip"). There are two 4K banks, selected via hotspots
  $1FF8/$1FF9 exactly as for plain F8; the RAM write port is $1000-$107F and
  the read port is $1080-$10FF.

  Reimplemented as a thin subclass of the CartridgeEnhanced-based F8: the
  only difference from F8 is the 128-byte RAM, so the constructor just sets
  the RAM size and lets the base class install the read/write ports.

  @author  Bradford W. Mott
*/
class CartridgeF8SC : public CartridgeF8
{
  friend class CartridgeF8SCWidget;

  public:
    CartridgeF8SC(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeF8SC();

  public:
    string name() const { return "CartridgeF8SC"; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeF8SC();
    CartridgeF8SC(const CartridgeF8SC&);
    CartridgeF8SC& operator=(const CartridgeF8SC&);
};

#endif
