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

#include "System.hxx"
#include "CartFC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFC::CartridgeFC(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : CartridgeEnhanced(image, size, settings, size),
    myTargetBank(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFC::~CartridgeFC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFC::reset()
{
  CartridgeEnhanced::reset();
  myTargetBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::checkSwitchBank(uint16_t address, uint8_t)
{
  if((address & ADDR_MASK) == 0x1FFC)
  {
    bank(myTargetBank);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::poke(uint16_t address, uint8_t value)
{
  switch(address & 0x0FFF)
  {
    case 0x0FF8:
      // Low two bits of the target bank
      myTargetBank = value & 0x03;
      break;

    case 0x0FF9:
      // Upper bits of the target bank
      if((uint16_t)(value << 2) < romBankCount())
      {
        myTargetBank += value << 2;
        myTargetBank %= romBankCount();
      }
      else
        myTargetBank = value % romBankCount();
      break;

    default:
      checkSwitchBank(address & ADDR_MASK, 0);
      break;
  }
  return false;
}
