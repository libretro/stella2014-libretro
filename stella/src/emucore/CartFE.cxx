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
#include "CartFE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::CartridgeFE(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 8192),
    myLastAddress1(0),
    myLastAddress2(0),
    myLastAddressChanged(false)
{
  // Switching follows the address bus directly, so pages route through
  // peek()/poke() rather than being direct-peeked.
  myDirectPeek = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::~CartridgeFE()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::reset()
{
  // Nothing to do: the bank follows the address bus, so there is no power-on
  // bank to select. (Overriding this also avoids CartridgeEnhanced::reset(),
  // whose bank() call would use the segment machinery this cart does not set
  // up in install().)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::install(System& system)
{
  mySystem = &system;

  // Map the whole 4K address space through this cart; peek() decodes the
  // active bank from the address itself. (No stack page is claimed.)
  uint16_t shift = mySystem->pageShift();
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint32_t addr = 0x1000; addr < 0x2000; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeFE::peek(uint16_t address)
{
  myLastAddress2 = myLastAddress1;
  myLastAddress1 = address;
  myLastAddressChanged = true;

  // Bank is selected by bit 13 of the accessed address: clear -> high bank.
  return myImage[(address & 0x0FFF) + (((address & 0x2000) == 0) ? 4096 : 0)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::poke(uint16_t, uint8_t)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeFE::bank() const
{
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
    myBankChanged = ((myLastAddress1 & 0x2000) == 0) !=
                    ((myLastAddress2 & 0x2000) == 0);
    myLastAddressChanged = false;
  }
  else
    myBankChanged = false;

  return Cartridge::bankChanged();
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
