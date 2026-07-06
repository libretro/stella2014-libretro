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

#include "Cart3EX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EX::Cartridge3EX(const uint8_t* image, uint32_t size,
                           const Settings& settings)
  : Cartridge3E(image, size, settings)
{
  // $FFFA holds (RAM bank count - 1). RAM banks are 1K each (half a 2K ROM
  // bank), so total RAM size follows from the bank count.
  myRamBankCount = (uint16_t)(image[size - 6] + 1);
  myRamSize = (uint32_t)(1 << (BANK_SHIFT_3E - 1)) * myRamBankCount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3EX::~Cartridge3EX()
{
}
