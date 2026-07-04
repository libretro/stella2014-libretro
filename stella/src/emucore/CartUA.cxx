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
// $Id: CartUA.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartUA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeUA::CartridgeUA(const uint8_t* image, uint32_t size, const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, MIN(8192u, size));
  createCodeAccessBase(8192);

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeUA::~CartridgeUA()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUA::reset()
{
  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeUA::install(System& system)
{
  mySystem     = &system;
  uint16_t shift = mySystem->pageShift();

  // Get the page accessing methods for the hot spots since they overlap
  // areas within the TIA we'll need to forward requests to the TIA
  myHotSpotPageAccess = mySystem->getPageAccess(0x0220 >> shift);

  // Set the page accessing methods for the hot spots
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  mySystem->setPageAccess(0x0220 >> shift, access);
  mySystem->setPageAccess(0x0240 >> shift, access);

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeUA::peek(uint16_t address)
{
  address &= 0x1FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0220:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;

    case 0x0240:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;

    default:
      break;
  }

  if(!(address & 0x1000))
  {
    return myHotSpotPageAccess.device->peek(address);
  }
  else
  {
    return 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::poke(uint16_t address, uint8_t value)
{
  address &= 0x1FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0220:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;

    case 0x0240:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;

    default:
      break;
  }

  if(!(address & 0x1000))
  {
    myHotSpotPageAccess.device->poke(address, value);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::bank(uint16_t bank)
{ 
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentBank = bank;
  uint16_t offset = myCurrentBank << 12;
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
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeUA::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeUA::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeUA::getImage(int& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::save(Serializer& out) const
{
   out.putString(name());
   out.putShort(myCurrentBank);

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeUA::load(Serializer& in)
{
   if(in.getString() != name())
      return false;

   myCurrentBank = in.getShort();

   // Remember what bank we were in
   bank(myCurrentBank);

   return true;
}
