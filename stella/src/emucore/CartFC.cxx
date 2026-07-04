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
#include "CartFC.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFC::CartridgeFC(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0),
    myTargetBank(0)
{
  // Allocate array for the ROM image
  myImage = new uint8_t[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);
  createCodeAccessBase(mySize);

  myNumBanks = (uint16_t)(mySize >> 12);
  if(myNumBanks < 1)
    myNumBanks = 1;

  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFC::~CartridgeFC()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFC::reset()
{
  myTargetBank = 0;
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFC::install(System& system)
{
  mySystem = &system;

  // Install pages for the startup bank
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFC::checkSwitchBank(uint16_t address, uint8_t value)
{
  switch(address)
  {
    case 0x0FF8:
      // Set the two lowest bits of the target 4K bank
      myTargetBank = value & 0x03;
      break;

    case 0x0FF9:
      // Set the high bits of the target 4K bank
      if((uint16_t)(value << 2) < myNumBanks)
      {
        myTargetBank += value << 2;
        myTargetBank %= myNumBanks;
      }
      else
        // Special handling when both values are identical (e.g. 4/4, 5/5)
        myTargetBank = value % myNumBanks;
      break;

    case 0x0FFC:
      // Commit the switch to the assembled target bank
      bank(myTargetBank);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeFC::peek(uint16_t address)
{
  uint16_t addr = address & 0x0FFF;

  checkSwitchBank(addr, 0);

  return myImage[(myCurrentBank << 12) + addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::poke(uint16_t address, uint8_t value)
{
  checkSwitchBank(address & 0x0FFF, value);
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::bank(uint16_t bank)
{
  if(bankLocked()) return false;

  if(bank >= myNumBanks) bank %= myNumBanks;

  myCurrentBank = bank;
  uint32_t offset = (uint32_t)myCurrentBank << 12;
  uint16_t shift = mySystem->pageShift();

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
uint16_t CartridgeFC::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeFC::bankCount() const
{
  return myNumBanks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::patch(uint16_t address, uint8_t value)
{
  myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeFC::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putShort(myTargetBank);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFC::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    myTargetBank = in.getShort();
  }
  catch(...)
  {
    return false;
  }

  bank(myCurrentBank);

  return true;
}
