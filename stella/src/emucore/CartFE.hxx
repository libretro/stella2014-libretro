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

#ifndef CARTRIDGEFE_HXX
#define CARTRIDGEFE_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Activision's FE bankswitching scheme (two 4K
  banks). FE has no hotspots: the bank is decoded from bit 13 of every
  accessed address, which flips as the ROM's JSR/RTS trampoline passes the
  return address through the mirror at $D000/$F000. This is the original
  2014 model, kept intact for behaviour and performance; it is only re-homed
  on CartridgeEnhanced so FE shares the common image handling and
  (de)serialization rather than being a bespoke Cartridge subclass.

  Because the bank follows the address bus directly, this cart overrides
  peek()/poke()/bank()/bankChanged() and does not use the base class's
  hotspot/segment machinery, the RIOT stack-snoop, or checkSwitchBank(); in
  particular it never claims the stack page, so normal stack traffic is not
  routed through the cart.

  @author  Bradford W. Mott
*/
class CartridgeFE : public CartridgeEnhanced
{
  friend class CartridgeFEWidget;

  public:
    CartridgeFE(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeFE();

  public:
    void reset();
    void install(System& system);

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

    uint16_t bank() const;
    uint16_t bankCount() const;
    bool bankChanged();

    bool save(Serializer& out) const;
    bool load(Serializer& in);

    string name() const { return "CartridgeFE"; }

  protected:
    // FE has no hotspot-style switching; the base class never needs to call
    // this, but it is pure virtual so a trivial implementation is provided.
    bool checkSwitchBank(uint16_t, uint8_t) { return false; }

  private:
    // Last two addresses accessed, used to detect a bank change (bit 13
    // toggling between successive accesses).
    uint16_t myLastAddress1, myLastAddress2;
    bool     myLastAddressChanged;

  private:
    // Following constructors and assignment operators not supported
    CartridgeFE();
    CartridgeFE(const CartridgeFE&);
    CartridgeFE& operator=(const CartridgeFE&);
};

#endif
