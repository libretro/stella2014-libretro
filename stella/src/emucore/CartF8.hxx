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

#ifndef CARTRIDGEF8_HXX
#define CARTRIDGEF8_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Atari's 8K bankswitched games. There are two 4K
  banks, selected by accessing hotspots $1FF8 (bank 0) and $1FF9 (bank 1).

  Reimplemented as a thin CartridgeEnhanced subclass (the base class provides
  the segment banking, page installation and (de)serialization); F8 only has
  to describe its two hotspots and its startup-bank selection.

  @author  Bradford W. Mott
*/
class CartridgeF8 : public CartridgeEnhanced
{
  friend class CartridgeF8Widget;

  public:
    CartridgeF8(const uint8_t* image, uint32_t size, const string& md5,
                const Settings& settings);
    virtual ~CartridgeF8();

  public:
    string name() const { return "CartridgeF8"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x1FF8; }

    uint16_t getStartBank() const { return myStartBankF8; }

  private:
    // The power-on bank, determined from the ROM's md5 in the constructor.
    // Most F8 games start in bank 1; a handful need bank 0.
    uint16_t myStartBankF8;

  private:
    // Following constructors and assignment operators not supported
    CartridgeF8();
    CartridgeF8(const CartridgeF8&);
    CartridgeF8& operator=(const CartridgeF8&);
};

#endif
