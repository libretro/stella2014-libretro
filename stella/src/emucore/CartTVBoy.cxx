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

#include "CartTVBoy.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeTVBoy::CartridgeTVBoy(const uint8_t* image, uint32_t size,
                               const Settings& settings)
  : CartridgeEnhanced(image, size, settings, size),
    myBankingDisabled(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeTVBoy::~CartridgeTVBoy()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::checkSwitchBank(uint16_t address, uint8_t)
{
  if((address & ADDR_MASK) >= 0x1800 && (address & ADDR_MASK) <= 0x187F)
  {
    bank(address & (romBankCount() - 1));
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::bank(uint16_t bank, uint16_t segment)
{
  if(myBankingDisabled) return false;

  bool banked = CartridgeEnhanced::bank(bank, segment);

  // The first switch to a non-zero bank locks all further switching
  if(banked && bank != 0)
    myBankingDisabled = true;

  return banked;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::save(Serializer& out) const
{
  CartridgeEnhanced::save(out);
  out.putBool(myBankingDisabled);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::load(Serializer& in)
{
  CartridgeEnhanced::load(in);
  myBankingDisabled = in.getBool();
  return true;
}
