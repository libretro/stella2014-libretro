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
#include "Cart03E0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge03E0::Cartridge03E0(const uint8_t* image, uint32_t size,
                             const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 8192)
{
  myBankShift = BANK_SHIFT_03E0;  // 1K slices -> four segments
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge03E0::~Cartridge03E0()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge03E0::reset()
{
  // The three switchable slices power on at banks 4, 5 and 6; the fourth
  // slice is fixed to the last 1K of the image by the base install.
  bank(4, 0);
  bank(5, 1);
  bank(6, 2);
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge03E0::install(System& system)
{
  CartridgeEnhanced::install(system);

  uint16_t shift = mySystem->pageShift();
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0380 >> shift);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x03c0 >> shift);

  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint32_t addr = 0x0380; addr < 0x03FF; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::checkSwitchBank(uint16_t address, uint8_t)
{
  bool switched = false;
  if((address & 0x10) == 0)
  {
    bank(address & 0x0007, 0);
    switched = true;
  }
  if((address & 0x20) == 0)
  {
    bank(address & 0x0007, 1);
    switched = true;
  }
  if((address & 0x40) == 0)
  {
    bank(address & 0x0007, 2);
    switched = true;
  }
  return switched;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t Cartridge03E0::peek(uint16_t address)
{
  checkSwitchBank(address, 0);
  int hotspot = ((address & 0x40) >> 6);
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::poke(uint16_t address, uint8_t value)
{
  if(!(address & 0x1000))
  {
    checkSwitchBank(address, 0);
    int hotspot = ((address & 0x40) >> 6);
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }
  return false;
}
