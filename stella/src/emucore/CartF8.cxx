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

#include "CartF8.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8::CartridgeF8(const uint8_t* image, uint32_t size,
                         const string& md5, const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 8192),
    myStartBankF8(1)
{
  // Most F8 games power on in bank 1, but a handful of titles rely on
  // powering on in bank 0.
  if(md5 == "bc24440b59092559a1ec26055fd1270e" ||  // Private Eye [a]
     md5 == "75ea60884c05ba496473c23a58edf12f" ||  // 8-in-1 Yars Revenge
     md5 == "75ee371ccfc4f43e7d9b8f24e1266b55" ||  // Snow White
     md5 == "74c8a6f20f8adaa7e05183f796eda796" ||  // Tricade Demo
     md5 == "9905f9f4706223dadee84f6867ede8e3")     // Challenge
    myStartBankF8 = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8::~CartridgeF8()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8::checkSwitchBank(uint16_t address, uint8_t)
{
  switch(address)
  {
    case 0x1FF8:
      bank(0);
      return true;
    case 0x1FF9:
      bank(1);
      return true;
    default:
      break;
  }
  return false;
}
