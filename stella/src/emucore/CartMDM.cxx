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

#include <cstring>

#include "System.hxx"
#include "CartMDM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDM::CartridgeMDM(const uint8_t* image, uint32_t size,
                           const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0),
    myBankingDisabled(false)
{
  // Allocate array for the ROM image
  myImage = new uint8_t[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize);

  myNumBanks = (uint16_t)(mySize >> 12);

  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMDM::~CartridgeMDM()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDM::reset()
{
  // A reset re-enables bankswitching and returns to bank 0
  myBankingDisabled = false;
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDM::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();

  // Get the page accessing methods for the hotspots, since they overlap
  // areas within the TIA/RIOT we'll need to forward requests to.
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0800 >> shift);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x0900 >> shift);
  myHotSpotPageAccess[2] = mySystem->getPageAccess(0x0A00 >> shift);
  myHotSpotPageAccess[3] = mySystem->getPageAccess(0x0B00 >> shift);
  myHotSpotPageAccess[4] = mySystem->getPageAccess(0x0C00 >> shift);
  myHotSpotPageAccess[5] = mySystem->getPageAccess(0x0D00 >> shift);
  myHotSpotPageAccess[6] = mySystem->getPageAccess(0x0E00 >> shift);
  myHotSpotPageAccess[7] = mySystem->getPageAccess(0x0F00 >> shift);

  System::PageAccess access(0, 0, 0, this, System::PA_READWRITE);

  // Set the page accessing methods for the hotspots ($0800-$0BFF)
  for(uint32_t addr = 0x0800; addr < 0x0BFF; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMDM::checkSwitchBank(uint16_t address)
{
  // Hotspots are $0800-$0BFF; the low byte selects the bank.
  if((address & 0x1C00) == 0x0800)
    bank(address & 0x00FF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeMDM::peek(uint16_t address)
{
  checkSwitchBank(address);

  // Because of the way accessing is set up, we only get here for accesses
  // in the hotspot region ($800-$BFF), which we forward.
  int hotspot = ((address & 0x0F00) >> 8) - 8;
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::poke(uint16_t address, uint8_t value)
{
  // We only care about accesses below $1000
  if(!(address & 0x1000))
  {
    checkSwitchBank(address);

    int hotspot = ((address & 0x0F00) >> 8) - 8;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::bank(uint16_t bank)
{
  if(bankLocked() || myBankingDisabled) return false;

  // Selecting bank 128 or above disables further bankswitching. Only the
  // low byte is used to index the ROM, so clamp to the available banks.
  if(bank > 127)
  {
    myBankingDisabled = true;
    return false;
  }

  myCurrentBank = bank % myNumBanks;
  uint32_t offset = (uint32_t)myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Map the selected 4K bank into the $1000-$1FFF ROM window
  for(uint32_t address = 0x1000; address < 0x2000; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeMDM::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeMDM::bankCount() const
{
  return myNumBanks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeMDM::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putBool(myBankingDisabled);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMDM::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    myBankingDisabled = in.getBool();
  }
  catch(...)
  {
    return false;
  }

  // Force the saved bank into the ROM window directly, bypassing the
  // banking-disabled/lock guards in bank().
  uint32_t offset = (uint32_t)myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint32_t address = 0x1000; address < 0x2000; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }

  return true;
}
