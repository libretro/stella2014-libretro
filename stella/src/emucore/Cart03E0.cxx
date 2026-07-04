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
#include "Cart03E0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge03E0::Cartridge03E0(const uint8_t* image, uint32_t size,
                             const Settings& settings)
  : Cartridge(settings),
    mySize(size)
{
  // Allocate array for the ROM image
  myImage = new uint8_t[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize);

  // Number of 1K banks
  myNumBanks = (uint16_t)(mySize >> 10);

  // The fourth segment is always the last 1K of the image
  myCurrentSlice[3] = myNumBanks - 1;

  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge03E0::~Cartridge03E0()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge03E0::reset()
{
  // Default the three switchable segments to banks 4, 5 and 6
  segment(4, 0);
  segment(5, 1);
  segment(6, 2);
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge03E0::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();

  // Get the page accessing methods for the hotspots, since they overlap
  // areas within the TIA we'll need to forward requests to.
  myHotSpotPageAccess[0] = mySystem->getPageAccess(0x0380 >> shift);
  myHotSpotPageAccess[1] = mySystem->getPageAccess(0x03c0 >> shift);

  // Set the page accessing methods for the hotspots ($0380-$03FF)
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint32_t addr = 0x0380; addr < 0x03FF; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);

  // Install the fixed fourth segment (last 1K), which never changes
  uint32_t offset3 = (uint32_t)myCurrentSlice[3] << 10;
  for(uint32_t address = 0x1C00; address < 0x2000; address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset3 + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset3 + (address & 0x03FF)];
    mySystem->setPageAccess(address >> shift, access);
  }

  // Set up the three switchable segments
  segment(4, 0);
  segment(5, 1);
  segment(6, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge03E0::segment(uint16_t bank, uint16_t seg)
{
  if(bankLocked()) return;

  bank &= 0x0007;
  myCurrentSlice[seg] = bank;
  uint32_t offset = (uint32_t)bank << 10;
  uint16_t shift = mySystem->pageShift();
  uint16_t base = (uint16_t)(0x1000 + (seg << 10));

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  for(uint32_t address = base; address < (uint32_t)(base + 0x0400);
      address += (1 << shift))
  {
    access.directPeekBase = &myImage[offset + (address & 0x03FF)];
    access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x03FF)];
    mySystem->setPageAccess(address >> shift, access);
  }

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge03E0::checkSwitchBank(uint16_t address)
{
  // A4 clear -> segment 0; A5 clear -> segment 1; A6 clear -> segment 2.
  // A0..A2 give the bank number.
  if((address & 0x10) == 0)
    segment(address & 0x0007, 0);
  if((address & 0x20) == 0)
    segment(address & 0x0007, 1);
  if((address & 0x40) == 0)
    segment(address & 0x0007, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t Cartridge03E0::peek(uint16_t address)
{
  checkSwitchBank(address);

  // Because of the way accessing is set up, we only get here for accesses
  // in the hotspot region (a TIA/RIOT read), which we forward.
  int hotspot = (address & 0x40) >> 6;
  return myHotSpotPageAccess[hotspot].device->peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::poke(uint16_t address, uint8_t value)
{
  // We may get here from a write to $0380-$03FF or to the cart; ignore any
  // cart write and forward the hotspot-region write to the TIA/RIOT.
  if(!(address & 0x1000))
  {
    checkSwitchBank(address);

    int hotspot = (address & 0x40) >> 6;
    myHotSpotPageAccess[hotspot].device->poke(address, value);
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::bank(uint16_t)
{
  // This scheme has no single "current bank"; segments are switched
  // individually via segment(). Provided only to satisfy the interface.
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t Cartridge03E0::bank() const
{
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t Cartridge03E0::bankCount() const
{
  // The scheme reports a single bank whose pieces can be swapped in many
  // different ways.
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::patch(uint16_t address, uint8_t value)
{
  address &= 0x0FFF;
  int segNum = address >> 10;
  uint32_t offset = (uint32_t)myCurrentSlice[segNum] << 10;
  myImage[offset + (address & 0x03FF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* Cartridge03E0::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShortArray(myCurrentSlice, 4);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge03E0::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    in.getShortArray(myCurrentSlice, 4);
  }
  catch(...)
  {
    return false;
  }

  // Restore the three switchable segments (the fourth is fixed and was
  // installed at power-on)
  segment(myCurrentSlice[0], 0);
  segment(myCurrentSlice[1], 1);
  segment(myCurrentSlice[2], 2);

  return true;
}
