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

#ifndef CARTRIDGEF4SC_HXX
#define CARTRIDGEF4SC_HXX

#include "bspf.hxx"
#include "CartF4.hxx"

/**
  Cartridge class used for Atari's 32K bankswitched games with 128 bytes of
  RAM (the "superchip"). Eight 4K banks via $1FF4-$1FFB, RAM write port
  $1000-$107F and read port $1080-$10FF.

  Reimplemented as a thin subclass of the CartridgeEnhanced-based F4: the
  only difference from F4 is the 128-byte RAM.

  @author  Bradford W. Mott
*/
class CartridgeF4SC : public CartridgeF4
{
  friend class CartridgeF4SCWidget;

  public:
    CartridgeF4SC(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeF4SC();

  public:
    string name() const { return "CartridgeF4SC"; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeF4SC();
    CartridgeF4SC(const CartridgeF4SC&);
    CartridgeF4SC& operator=(const CartridgeF4SC&);
};

#endif
