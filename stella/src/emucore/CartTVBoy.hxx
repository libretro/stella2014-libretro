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

#ifndef CARTRIDGE_TVBOY_HXX
#define CARTRIDGE_TVBOY_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for the "TV Boy" 128-in-1 (and similar) multicarts.
  The image is a number of 4K banks; a write in $1800-$187F selects a bank
  from the low bits of the address. After the first real bankswitch (to a
  non-zero bank) further switching is locked out, which is how the built-in
  menu hands control to the selected game.

  Reimplemented on CartridgeEnhanced. To preserve the current core's exact
  behaviour, this cart deliberately does NOT declare a hotspot(): the whole
  ROM window stays direct-peeked, so the bankswitch is triggered by writes
  only (matching the standalone version, whose read hotspot was likewise
  masked by direct peek). checkSwitchBank() therefore runs only from poke().

  @author  Fabio Spadaro
*/
class CartridgeTVBoy : public CartridgeEnhanced
{
  friend class CartridgeTVBoyWidget;

  public:
    CartridgeTVBoy(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeTVBoy();

  public:
    bool save(Serializer& out) const;
    bool load(Serializer& in);

    string name() const { return "CartridgeTVBoy"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    bool bank(uint16_t bank, uint16_t segment);
    bool bank(uint16_t bank_) { return this->bank(bank_, 0); }

  private:
    // After the first non-zero bankswitch, all further switching is disabled
    bool myBankingDisabled;

  private:
    // Following constructors and assignment operators not supported
    CartridgeTVBoy();
    CartridgeTVBoy(const CartridgeTVBoy&);
    CartridgeTVBoy& operator=(const CartridgeTVBoy&);
};

#endif
