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
// $Id: CartFE.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartFE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::CartridgeFE(const uint8_t* image, uint32_t size, const Settings& settings)
  : Cartridge(settings),
    myLastAddress1(0),
    myLastAddress2(0),
    myLastAddressChanged(false)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, MIN(8192u, size));

  // We use System::PageAccess.codeAccessBase, but don't allow its use
  // through a pointer, since the address space of FE carts can change
  // at the instruction level, and PageAccess is normally defined at an
  // interval of 64 bytes
  //
  createCodeAccessBase(8192);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::~CartridgeFE()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::reset()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::install(System& system)
{
  mySystem     = &system;
  uint16_t shift = mySystem->pageShift();

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  // Map all of the accesses to call peek and poke
  for(uint32_t i = 0x1000; i < 0x2000; i += (1 << shift))
    mySystem->setPageAccess(i >> shift, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeFE::peek(uint16_t address)
{
  // The bank is determined by A13 of the processor
  // We keep track of the two most recent accesses to determine which bank
  // we're in, and when the values actually changed
  myLastAddress2 = myLastAddress1;
  myLastAddress1 = address;
  myLastAddressChanged = true;

  return myImage[(address & 0x0FFF) + (((address & 0x2000) == 0) ? 4096 : 0)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::poke(uint16_t, uint8_t)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::bank(uint16_t)
{
  // Doesn't support bankswitching in the normal sense
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeFE::bank() const
{
  // The current bank depends on the last address accessed
  return ((myLastAddress1 & 0x2000) == 0) ? 1 : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeFE::bankCount() const
{
  return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::bankChanged()
{
  if(myLastAddressChanged)
  {
    // A bankswitch occurs when the addresses transition from state to another
    myBankChanged = ((myLastAddress1 & 0x2000) == 0) !=
                    ((myLastAddress2 & 0x2000) == 0);
    myLastAddressChanged = false;
  }
  else
    myBankChanged = false;

  // In any event, let the base class know about it
  return Cartridge::bankChanged();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::patch(uint16_t address, uint8_t value)
{
  myImage[(address & 0x0FFF) + (((address & 0x2000) == 0) ? 4096 : 0)] = value;
  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeFE::getImage(int& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::save(Serializer& out) const
{
   out.putString(name());
   out.putShort(myLastAddress1);
   out.putShort(myLastAddress2);

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::load(Serializer& in)
{
   if(in.getString() != name())
      return false;

   myLastAddress1 = in.getShort();
   myLastAddress2 = in.getShort();

   return true;
}
