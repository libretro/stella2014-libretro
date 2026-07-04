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
#include "CartTVBoy.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeTVBoy::CartridgeTVBoy(const uint8_t* image, uint32_t size,
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

  // The menu is in bank 0 at power-on
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeTVBoy::~CartridgeTVBoy()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoy::reset()
{
  myBankingDisabled = false;
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoy::install(System& system)
{
  mySystem = &system;

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeTVBoy::checkSwitchBank(uint16_t address)
{
  // Hotspots are $1800-$187F (masked here to $800-$87F). The low bits of
  // the address form the bank number.
  if(address >= 0x0800 && address <= 0x087F)
    bank(address & (myNumBanks - 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeTVBoy::peek(uint16_t address)
{
  address &= 0x0FFF;

  checkSwitchBank(address);

  return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::poke(uint16_t address, uint8_t)
{
  checkSwitchBank(address & 0x0FFF);
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::bank(uint16_t bank)
{
  // Any bankswitching after the first one is locked out; the check for
  // bank 0 avoids locking on the power-on install.
  if(myBankingDisabled) return false;

  if(bank >= myNumBanks) return false;

  // Remember what bank we're in
  myCurrentBank = bank;
  uint32_t offset = myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();

  // Setup the page access methods for the current bank
  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Map ROM image into the system
  for(uint32_t address = 0x1000; address < 0x2000; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }

  // Any bankswitching locks further bankswitching (but not the initial
  // power-on selection of bank 0)
  if(bank != 0)
    myBankingDisabled = true;

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeTVBoy::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeTVBoy::bankCount() const
{
  return myNumBanks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeTVBoy::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeTVBoy::save(Serializer& out) const
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
bool CartridgeTVBoy::load(Serializer& in)
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

  // Remember what bank we were in (force the mapping, bypassing the lock)
  uint32_t offset = myCurrentBank << 12;
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
