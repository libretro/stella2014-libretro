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

#ifndef CARTRIDGEFA_HXX
#define CARTRIDGEFA_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for CBS' RAM Plus games. There are three 4K banks,
  selected via hotspots $1FF8/$1FF9/$1FFA, plus 256 bytes of RAM: the write
  port is $1000-$10FF and the read port is $1100-$11FF.

  Reimplemented as a thin CartridgeEnhanced subclass: the base class provides
  the banking and the RAM read/write ports; FA only describes its three
  hotspots, its 256-byte RAM and its power-on bank (2).

  @author  Bradford W. Mott
*/
class CartridgeFA : public CartridgeEnhanced
{
  friend class CartridgeFAWidget;

  public:
    CartridgeFA(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeFA();

  public:
    string name() const { return "CartridgeFA"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x1FF8; }

    uint16_t getStartBank() const { return 2; }

  private:
    // Following constructors and assignment operators not supported
    CartridgeFA();
    CartridgeFA(const CartridgeFA&);
    CartridgeFA& operator=(const CartridgeFA&);
};

#endif
