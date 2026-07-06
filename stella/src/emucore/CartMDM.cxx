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
#include "CartMDM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDM::CartridgeMDM(const uint8_t* image, uint32_t size,
                           const Settings& settings)
  : CartridgeEnhanced(image, size, settings, size),
    myBankingDisabled(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDM::~CartridgeMDM()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDM::install(System& system)
{
  CartridgeEnhanced::install(system);

  uint16_t shift = mySystem->pageShift();
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0800 >> shift);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0900 >> shift);
  myHotSpotPageAccess[2] = mySystem->getPageAccess(0x0A00 >> shift);
  myHotSpotPageAccess[3] = mySystem->getPageAccess(0x0B00 >> shift);
  myHotSpotPageAccess[4] = mySystem->getPageAccess(0x0C00 >> shift);
  myHotSpotPageAccess[5] = mySystem->getPageAccess(0x0D00 >> shift);
  myHotSpotPageAccess[6] = mySystem->getPageAccess(0x0E00 >> shift);
  myHotSpotPageAccess[7] = mySystem->getPageAccess(0x0F00 >> shift);

  System::PageAccess access(0, 0, 0, this, System::PA_READWRITE);
  for(uint32_t addr = 0x0800; addr < 0x0BFF; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::checkSwitchBank(uint16_t address, uint8_t)
{
  if((address & 0x1C00) == 0x0800)
  {
    bank(address & 0x00FF);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::bank(uint16_t bank, uint16_t segment)
{
  if(bankLocked() || myBankingDisabled) return false;

  // Selecting a bank above 127 disables all further switching
  if(bank > 127)
  {
    myBankingDisabled = true;
    return false;
  }

  return CartridgeEnhanced::bank(bank, segment);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeMDM::peek(uint16_t address)
{
  checkSwitchBank(address, 0);
  int hotspot = ((address & 0x0F00) >> 8) - 8;
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::poke(uint16_t address, uint8_t value)
{
  if(!(address & 0x1000))
  {
    checkSwitchBank(address, 0);
    int hotspot = ((address & 0x0F00) >> 8) - 8;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::save(Serializer& out) const
{
  CartridgeEnhanced::save(out);
  out.putBool(myBankingDisabled);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::load(Serializer& in)
{
  CartridgeEnhanced::load(in);
  myBankingDisabled = in.getBool();
  return true;
}
