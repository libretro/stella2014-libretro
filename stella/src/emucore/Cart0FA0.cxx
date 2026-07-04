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
#include "Cart0FA0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge0FA0::Cartridge0FA0(const uint8_t* image, uint32_t size,
                             const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0)
{
  // Allocate array for the ROM image (always 8K for this scheme)
  myImage = new uint8_t[8192];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, size < 8192 ? size : 8192);
  createCodeAccessBase(8192);

  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge0FA0::~Cartridge0FA0()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge0FA0::reset()
{
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge0FA0::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();

  // Get the page accessing method for the hotspot region, since it
  // overlaps the TIA/RIOT area we'll need to forward requests to.
  myHotSpotPageAccess = mySystem->getPageAccess(0x06a0 >> shift);

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page accessing methods for the hotspots and their mirrors.
  // A11 and A8 are not decoded, so map all four combinations.
  for(uint16_t a11 = 0; a11 <= 1; ++a11)
  {
    for(uint16_t a8 = 0; a8 <= 1; ++a8)
    {
      uint16_t addr = (uint16_t)((a11 << 11) + (a8 << 8));
      mySystem->setPageAccess((0x06a0 | addr) >> shift, access);
      mySystem->setPageAccess((0x06c0 | addr) >> shift, access);
    }
  }

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge0FA0::checkSwitchBank(uint16_t address)
{
  // A10, A9 and A7 are the fixed part of the hotspot address; A6/A5 select
  // the bank. Mask to the decoded bits and compare.
  switch(address & 0x16e0)
  {
    case 0x06a0:
      bank(0);   // lower 4K bank
      break;
    case 0x06c0:
      bank(1);   // upper 4K bank
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t Cartridge0FA0::peek(uint16_t address)
{
  checkSwitchBank(address);

  // Because of the way accessing is set up, we only get here for accesses
  // in the hotspot region (a TIA/RIOT read), which we forward.
  return myHotSpotPageAccess.device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::poke(uint16_t address, uint8_t value)
{
  checkSwitchBank(address);

  // Forward the write to the underlying TIA/RIOT device; ignore any write
  // that would land in the cart's own ROM window.
  if(!(address & 0x1000))
    myHotSpotPageAccess.device->poke(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::bank(uint16_t bank)
{
  if(bankLocked()) return false;

  myCurrentBank = bank & 0x1;
  uint32_t offset = myCurrentBank << 12;
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
uint16_t Cartridge0FA0::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t Cartridge0FA0::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* Cartridge0FA0::getImage(int& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge0FA0::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
  }
  catch(...)
  {
    return false;
  }

  // Restore the active bank
  bank(myCurrentBank);

  return true;
}
