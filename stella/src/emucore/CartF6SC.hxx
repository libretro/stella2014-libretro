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

#ifndef CARTRIDGEF6SC_HXX
#define CARTRIDGEF6SC_HXX

#include "bspf.hxx"
#include "CartF6.hxx"

/**
  Cartridge class used for Atari's 16K bankswitched games with 128 bytes of
  RAM (the "superchip"). Four 4K banks via $1FF6-$1FF9, RAM write port
  $1000-$107F and read port $1080-$10FF.

  Reimplemented as a thin subclass of the CartridgeEnhanced-based F6: the
  only difference from F6 is the 128-byte RAM.

  @author  Bradford W. Mott
*/
class CartridgeF6SC : public CartridgeF6
{
  friend class CartridgeF6SCWidget;

  public:
    CartridgeF6SC(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeF6SC();

  public:
    string name() const { return "CartridgeF6SC"; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeF6SC();
    CartridgeF6SC(const CartridgeF6SC&);
    CartridgeF6SC& operator=(const CartridgeF6SC&);
};

#endif
