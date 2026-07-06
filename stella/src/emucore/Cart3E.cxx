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

#include "System.hxx"
#include "TIA.hxx"
#include "Cart3E.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3E::Cartridge3E(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : CartridgeEnhanced(image, size, settings, size)
{
  myBankShift    = BANK_SHIFT_3E;      // 2K ROM segments
  myRamSize      = RAM_SIZE_3E;        // 32K total RAM
  myRamBankCount = RAM_BANKS_3E;       // 32 x 1K RAM banks
  myRamWpHigh    = true;               // write port is the high half
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3E::~Cartridge3E()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::install(System& system)
{
  CartridgeEnhanced::install(system);

  uint16_t shift = mySystem->pageShift();

  // The hotspots ($3E and $3F) are in TIA address space, so we claim those
  // pages here to see the writes (and chain them on to the TIA).
  System::PageAccess access(0, 0, 0, this, System::PA_READWRITE);
  for(uint32_t addr = 0x00; addr < 0x40; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::checkSwitchBank(uint16_t address, uint8_t value)
{
  // Switch banks if necessary
  if(address == 0x003F)
  {
    // Switch ROM bank into the low segment
    bank(value);
    return true;
  }
  else if(address == 0x003E)
  {
    // Switch RAM bank into the low segment
    bank(value + romBankCount());
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t Cartridge3E::peek(uint16_t address)
{
  uint16_t peekAddress = address;
  address &= ROM_MASK;

  if(address < 0x0040)  // TIA access
    return mySystem->tia().peek(address);

  return CartridgeEnhanced::peek(peekAddress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::poke(uint16_t address, uint8_t value)
{
  uint16_t pokeAddress = address;
  address &= ROM_MASK;

  if(address < 0x0040)  // TIA access
  {
    checkSwitchBank(address, value);
    return mySystem->tia().poke(address, value);
  }

  return CartridgeEnhanced::poke(pokeAddress, value);
}
