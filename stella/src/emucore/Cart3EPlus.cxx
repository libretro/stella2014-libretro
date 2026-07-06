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
#include "Cart3EPlus.hxx"

namespace {
  // bankswitch size = max(4K, size rounded up to the next 1K)
  static uint32_t threeEPlusBsSize(uint32_t size)
  {
    uint32_t rounded = (size + 0x3FF) & ~0x3FFu;   // round up to 1K
    return rounded < 0x1000 ? 0x1000 : rounded;    // at least 4K
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EPlus::Cartridge3EPlus(const uint8_t* image, uint32_t size,
                                 const Settings& settings)
  : Cartridge3E(image, size, settings, threeEPlusBsSize(size))
{
  myBankShift    = BANK_SHIFT_3EP;    // 1K ROM segments
  myRamSize      = RAM_SIZE_3EP;      // 32K total RAM
  myRamBankCount = RAM_BANKS_3EP;     // 64 x 512-byte RAM banks
  myRamWpHigh    = true;              // inherited 3E behaviour
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EPlus::~Cartridge3EPlus()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3EPlus::reset()
{
  CartridgeEnhanced::reset();

  // Segments 0-2 start on random ROM banks; segment 3 holds the reset bank
  // (which contains the reset vectors).
  bank(mySystem->randGenerator().next() % romBankCount(), 0);
  bank(mySystem->randGenerator().next() % romBankCount(), 1);
  bank(mySystem->randGenerator().next() % romBankCount(), 2);
  bank(myStartBank, 3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3EPlus::checkSwitchBank(uint16_t address, uint8_t value)
{
  // Switch banks if necessary. The low 6 bits select the bank; the top 2
  // bits of the written value select which 1K segment to map it into.
  if(address == 0x003F)
  {
    // Switch a ROM bank into the addressed segment
    bank(value & 0x3F, value >> 6);
    return true;
  }
  else if(address == 0x003E)
  {
    // Switch a RAM bank into the addressed segment
    bank((value & 0x3F) + romBankCount(), value >> 6);
    return true;
  }
  return false;
}
