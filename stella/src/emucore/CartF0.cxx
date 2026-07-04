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
// $Id: CartF0.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartF0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF0::CartridgeF0(const uint8_t* image, uint32_t size, const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, MIN(65536u, size));
  createCodeAccessBase(65536);

  // Remember startup bank
  myStartBank = 15;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF0::~CartridgeF0()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF0::reset()
{
  // Upon reset we switch to bank 15
  myCurrentBank = 14;
  incbank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF0::install(System& system)
{
  mySystem = &system;

  // Install pages for bank 1
  myCurrentBank = 0;
  incbank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeF0::peek(uint16_t address)
{
  address &= 0x0FFF;

  // Switch to next bank
  if(address == 0x0FF0)
    incbank();

  return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::poke(uint16_t address, uint8_t)
{
  address &= 0x0FFF;

  // Switch to next bank
  if(address == 0x0FF0)
    incbank();

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF0::incbank()
{
  if(bankLocked()) return;

  // Remember what bank we're in
  myCurrentBank++;
  myCurrentBank &= 0x0F;
  uint16_t offset = myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();
  uint16_t mask = mySystem->pageMask();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uint32_t i = (0x1FF0 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
    mySystem->setPageAccess(i >> shift, access);
  }

  // Setup the page access methods for the current bank
  for(uint32_t address = 0x1000; address < (0x1FF0U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::bank(uint16_t bank)
{
  if(bankLocked()) return false;

  myCurrentBank = bank - 1;
  incbank();

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeF0::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeF0::bankCount() const
{
  return 16;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeF0::getImage(int& size) const
{
  size = 65536;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::save(Serializer& out) const
{
   out.putString(name());
   out.putShort(myCurrentBank);

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF0::load(Serializer& in)
{
   if(in.getString() != name())
      return false;

   myCurrentBank = in.getShort();

   // Remember what bank we were in
   --myCurrentBank;
   incbank();

   return true;
}
