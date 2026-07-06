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
#include "Cart0FA0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge0FA0::Cartridge0FA0(const uint8_t* image, uint32_t size,
                             const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 8192)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge0FA0::~Cartridge0FA0()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge0FA0::install(System& system)
{
  CartridgeEnhanced::install(system);

  uint16_t shift = mySystem->pageShift();
  myHotSpotPageAccess = mySystem->getPageAccess(0x06a0 >> shift);

  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint16_t a11 = 0; a11 <= 1; ++a11)
  {
    for(uint16_t a8 = 0; a8 <= 1; ++a8)
    {
      uint16_t addr = (a11 << 11) + (a8 << 8);
      mySystem->setPageAccess((0x06a0 | addr) >> shift, access);
      mySystem->setPageAccess((0x06c0 | addr) >> shift, access);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::checkSwitchBank(uint16_t address, uint8_t)
{
  switch(address & 0x16e0)
  {
    case 0x06a0:
      bank(0);  // lower 4K bank
      return true;
    case 0x06c0:
      bank(1);  // upper 4K bank
      return true;
    default:
      break;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t Cartridge0FA0::peek(uint16_t address)
{
  address &= myBankMask;
  checkSwitchBank(address, 0);
  return myHotSpotPageAccess.device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::poke(uint16_t address, uint8_t value)
{
  address &= myBankMask;
  checkSwitchBank(address, 0);
  if(!(address & 0x1000))
    myHotSpotPageAccess.device->poke(address, value);
  return false;
}
