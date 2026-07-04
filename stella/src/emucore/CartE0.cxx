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
// $Id: CartE0.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartE0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0::CartridgeE0(const uint8_t* image, uint32_t size, const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, MIN(8192u, size));
  createCodeAccessBase(8192);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0::~CartridgeE0()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::reset()
{
  // Setup segments to some default slices.
  //
  // On real hardware the three switchable 1K segments power up pointing at
  // indeterminate banks; a game that reads a segment before selecting it
  // sees whatever was there. When power-up randomization is enabled, pick
  // random start banks (as Stella 7 does) so this indeterminacy is modelled;
  // otherwise fall back to the fixed 4/5/6 the core has always used. Gated
  // on the same "ramrandom" setting that already controls RAM power-up
  // randomization, and kept deterministic via the emulated RNG.
  if(mySettings.getBool("ramrandom"))
  {
    segmentZero(mySystem->randGenerator().next() % 8);
    segmentOne(mySystem->randGenerator().next() % 8);
    segmentTwo(mySystem->randGenerator().next() % 8);
  }
  else
  {
    segmentZero(4);
    segmentOne(5);
    segmentTwo(6);
  }

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();
  uint16_t mask = mySystem->pageMask();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Set the page acessing methods for the first part of the last segment
  for(uint32_t i = 0x1C00; i < (0x1FE0U & ~mask); i += (1 << shift))
  {
    access.directPeekBase = &myImage[7168 + (i & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[7168 + (i & 0x03FF)];
    mySystem->setPageAccess(i >> shift, access);
  }
  myCurrentSlice[3] = 7;

  // Set the page accessing methods for the hot spots in the last segment
  access.directPeekBase = 0;
  access.codeAccessBase = &myCodeAccessBase[8128];
  access.type = System::PA_READ;
  for(uint32_t j = (0x1FE0 & ~mask); j < 0x2000; j += (1 << shift))
    mySystem->setPageAccess(j >> shift, access);

  // Install some default slices for the other segments
  segmentZero(4);
  segmentOne(5);
  segmentTwo(6);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeE0::peek(uint16_t address)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    segmentZero(address & 0x0007);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    segmentOne(address & 0x0007);
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    segmentTwo(address & 0x0007);
  }

  return myImage[(myCurrentSlice[address >> 10] << 10) + (address & 0x03FF)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::poke(uint16_t address, uint8_t)
{
  address &= 0x0FFF;

  // Switch banks if necessary
  if((address >= 0x0FE0) && (address <= 0x0FE7))
  {
    segmentZero(address & 0x0007);
  }
  else if((address >= 0x0FE8) && (address <= 0x0FEF))
  {
    segmentOne(address & 0x0007);
  }
  else if((address >= 0x0FF0) && (address <= 0x0FF7))
  {
    segmentTwo(address & 0x0007);
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::segmentZero(uint16_t slice)
{ 
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentSlice[0] = slice;
  uint16_t offset = slice << 10;
  uint16_t shift = mySystem->pageShift();

  // Setup the page access methods for the current bank
  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  for(uint32_t address = 0x1000; address < 0x1400; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::segmentOne(uint16_t slice)
{ 
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentSlice[1] = slice;
  uint16_t offset = slice << 10;
  uint16_t shift = mySystem->pageShift();

  // Setup the page access methods for the current bank
  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  for(uint32_t address = 0x1400; address < 0x1800; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::segmentTwo(uint16_t slice)
{ 
  if(bankLocked()) return;

  // Remember the new slice
  myCurrentSlice[2] = slice;
  uint16_t offset = slice << 10;
  uint16_t shift = mySystem->pageShift();

  // Setup the page access methods for the current bank
  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  for(uint32_t address = 0x1800; address < 0x1C00; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::bank(uint16_t)
{
  // Doesn't support bankswitching in the normal sense
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeE0::bank() const
{
  // Doesn't support bankswitching in the normal sense
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeE0::bankCount() const
{
  // Doesn't support bankswitching in the normal sense
  // There is one 'virtual' bank that can change in many different ways
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::patch(uint16_t address, uint8_t value)
{
  address &= 0x0FFF;
  myImage[(myCurrentSlice[address >> 10] << 10) + (address & 0x03FF)] = value;
  return true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeE0::getImage(int& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::save(Serializer& out) const
{
  out.putString(name());
  out.putShortArray(myCurrentSlice, 4);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::load(Serializer& in)
{
  if(in.getString() != name())
    return false;

  in.getShortArray(myCurrentSlice, 4);
  return true;
}
