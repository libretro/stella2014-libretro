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
#include "CartE0.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0::CartridgeE0(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 8192)
{
  myBankShift = BANK_SHIFT_E0;   // 1K segments -> four slices
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE0::~CartridgeE0()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE0::reset()
{
  // Slice 3 is fixed to the last segment (7); CartridgeEnhanced::install()
  // has already pointed it there. On real hardware the three switchable
  // slices power up at indeterminate segments, so when power-up
  // randomization is enabled we pick random start segments (matching the
  // standalone core and Stella 7), consuming the emulated RNG in the same
  // order; otherwise we fall back to the fixed 4/5/6 the core has always
  // used. This must mirror the old CartE0 exactly for bit-identical output.
  if(mySettings.getBool("ramrandom"))
  {
    bank(mySystem->randGenerator().next() % 8, 0);
    bank(mySystem->randGenerator().next() % 8, 1);
    bank(mySystem->randGenerator().next() % 8, 2);
  }
  else
  {
    bank(4, 0);
    bank(5, 1);
    bank(6, 2);
  }

  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeE0::checkSwitchBank(uint16_t address, uint8_t)
{
  address &= ROM_MASK;

  if(address >= 0x0FE0 && address <= 0x0FE7)
  {
    bank(address & 0x0007, 0);
    return true;
  }
  else if(address >= 0x0FE8 && address <= 0x0FEF)
  {
    bank(address & 0x0007, 1);
    return true;
  }
  else if(address >= 0x0FF0 && address <= 0x0FF7)
  {
    bank(address & 0x0007, 2);
    return true;
  }
  return false;
}
