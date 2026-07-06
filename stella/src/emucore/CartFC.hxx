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

#ifndef CARTRIDGE_FC_HXX
#define CARTRIDGE_FC_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Amiga's Power Play Arcade Video Game Album. The
  image is a set of 4K banks selected by a two-step sequence: a write to $FF8
  latches the low two bits of the target bank, a write to $FF9 the upper bits,
  and a write to $FFC commits the switch.

  Reimplemented on CartridgeEnhanced. To stay byte-identical to the current
  core this port does NOT declare a hotspot(): the ROM window is fully
  direct-peeked, so the sequence is driven by writes only (as before), and the
  latch/commit logic lives in poke().

  @author  Thomas Jentzsch (original); 2014 port
*/
class CartridgeFC : public CartridgeEnhanced
{
  friend class CartridgeFCWidget;

  public:
    CartridgeFC(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeFC();

  public:
    void reset();

    bool poke(uint16_t address, uint8_t value);

    string name() const { return "CartridgeFC"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

  private:
    // The bank latched by the $FF8/$FF9 sequence, committed on $FFC
    uint16_t myTargetBank;

  private:
    // Following constructors and assignment operators not supported
    CartridgeFC();
    CartridgeFC(const CartridgeFC&);
    CartridgeFC& operator=(const CartridgeFC&);
};

#endif
