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
// $Id: CartF8SC.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartF8SC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8SC::CartridgeF8SC(const uint8_t* image, uint32_t size, const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, MIN(8192u, size));
  createCodeAccessBase(8192);

  // This cart contains 128 bytes extended RAM @ 0x1000
  registerRamArea(0x1000, 128, 0x80, 0x00);

  // Remember startup bank
  myStartBank = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8SC::~CartridgeF8SC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8SC::reset()
{
  // Initialize RAM
  if(mySettings.getBool("ramrandom"))
    for(uint32_t i = 0; i < 128; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    memset(myRAM, 0, 128);

  // Upon reset we switch to the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8SC::install(System& system)
{
  mySystem     = &system;
  uint16_t shift = mySystem->pageShift();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page accessing method for the RAM writing pages
  access.type = System::PA_WRITE;
  for(uint32_t j = 0x1000; j < 0x1080; j += (1 << shift))
  {
    access.directPokeBase = &myRAM[j & 0x007F];
    access.codeAccessBase = &myCodeAccessBase[j & 0x007F];
    mySystem->setPageAccess(j >> shift, access);
  }

  // Set the page accessing method for the RAM reading pages
  access.directPokeBase = 0;
  access.type = System::PA_READ;
  for(uint32_t k = 0x1080; k < 0x1100; k += (1 << shift))
  {
    access.directPeekBase = &myRAM[k & 0x007F];
    access.codeAccessBase = &myCodeAccessBase[0x80 + (k & 0x007F)];
    mySystem->setPageAccess(k >> shift, access);
  }

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeF8SC::peek(uint16_t address)
{
  uint16_t peekAddress = address;
  address &= 0x0FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0FF8:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;
  
    case 0x0FF9:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;
  
    default:
      break;
  }

  if(address < 0x0080)  // Write port is at 0xF000 - 0xF080 (128 bytes)
  {
    // Reading from the write port triggers an unwanted write
    uint8_t value = mySystem->getDataBusState(0xFF);

    if(bankLocked())
      return value;
    else
    {
      triggerReadFromWritePort(peekAddress);
      return myRAM[address] = value;
    }
  }  
  else
    return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::poke(uint16_t address, uint8_t)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  switch(address)
  {
    case 0x0FF8:
      // Set the current bank to the lower 4k bank
      bank(0);
      break;
  
    case 0x0FF9:
      // Set the current bank to the upper 4k bank
      bank(1);
      break;
  
    default:
      break;
  }

  // NOTE: This does not handle accessing RAM, however, this function
  // should never be called for RAM because of the way page accessing
  // has been setup
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::bank(uint16_t bank)
{ 
  if(bankLocked()) return false;

  // Remember what bank we're in
  myCurrentBank = bank;
  uint16_t offset = myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();
  uint16_t mask = mySystem->pageMask();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page accessing methods for the hot spots
  for(uint32_t i = (0x1FF8 & ~mask); i < 0x2000; i += (1 << shift))
  {
    access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
    mySystem->setPageAccess(i >> shift, access);
  }

  // Setup the page access methods for the current bank
  for(uint32_t address = 0x1100; address < (0x1FF8U & ~mask);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeF8SC::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeF8SC::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::patch(uint16_t address, uint8_t value)
{
  address &= 0x0FFF;

  if(address < 0x0100)
  {
    // Normally, a write to the read port won't do anything
    // However, the patch command is special in that ignores such
    // cart restrictions
    myRAM[address & 0x007F] = value;
  }
  else
    myImage[(myCurrentBank << 12) + address] = value;

  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeF8SC::getImage(int& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::save(Serializer& out) const
{
   out.putString(name());
   out.putShort(myCurrentBank);
   out.putByteArray(myRAM, 128);

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8SC::load(Serializer& in)
{
   if(in.getString() != name())
      return false;

   myCurrentBank = in.getShort();
   in.getByteArray(myRAM, 128);

   // Remember what bank we were in
   bank(myCurrentBank);

   return true;
}
