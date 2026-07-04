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
//
// $Id: CartBF.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartBF.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBF::CartridgeBF(const uint8_t* image, uint32_t size, const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, MIN(262144u, size));
  createCodeAccessBase(262144);

  // Remember startup bank
  myStartBank = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBF::~CartridgeBF()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBF::reset()
{
  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBF::install(System& system)
{
  mySystem = &system;

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeBF::peek(uint16_t address)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0F80) && (address <= 0x0FBF))
    bank(address - 0x0F80);

  return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBF::poke(uint16_t address, uint8_t)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0F80) && (address <= 0x0FBF))
    bank(address - 0x0F80);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBF::bank(uint16_t bank)
{
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentBank = bank;
  uint32_t offset = myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();
  uint16_t mask = mySystem->pageMask();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uint32_t i = (0x1F80 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
    mySystem->setPageAccess(i >> shift, access);
  }

  // Setup the page access methods for the current bank
  for(uint32_t address = 0x1000; address < (0x1F80U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeBF::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeBF::bankCount() const
{
  return 64;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBF::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeBF::getImage(int& size) const
{
  size = 262144;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBF::save(Serializer& out) const
{
   out.putString(name());
   out.putShort(myCurrentBank);

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBF::load(Serializer& in)
{
   if(in.getString() != name())
      return false;

   myCurrentBank = in.getShort();

   // Remember what bank we were in
   bank(myCurrentBank);

   return true;
}
