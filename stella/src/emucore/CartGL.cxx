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
#include "CartGL.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeGL::CartridgeGL(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 4096),
    myInitialRAM(0),
    myEnablePROM(false)
{
  myBankShift    = BANK_SHIFT_GL;
  myRamBankShift = BANK_SHIFT_GL;
  myRamSize      = RAM_SIZE_GL;
  myRamBankCount = RAM_BANKS_GL;

  // A 6K image carries the initial RAM contents in its top 2K
  if(size == 4096 + 2048)
  {
    myInitialRAM = new uint8_t[2048];
    memcpy(myInitialRAM, image + 4096, 2048);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeGL::~CartridgeGL()
{
  delete[] myInitialRAM;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeGL::reset()
{
  CartridgeEnhanced::reset();

  // All four slices power on at ROM bank 0
  bank(0, 0);
  bank(0, 1);
  bank(0, 2);
  bank(0, 3);
  myBankChanged = true;

  // Remember the page access at the PROM window so it can be restored
  myOrgAccess = mySystem->getPageAccess(0x1FC0 >> mySystem->pageShift());

  // Random-fill RAM, then overlay initial RAM data if the image supplied it
  if(myRamSize > 0)
    for(uint32_t i = 0; i < myRamSize; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  if(myInitialRAM != 0)
    memcpy(myRAM, myInitialRAM, 2048);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeGL::install(System& system)
{
  CartridgeEnhanced::install(system);

  // Claim the hotspot pages so the cart sees the slice/control accesses
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  mySystem->setPageAccess(0x0480 >> mySystem->pageShift(), access);
  mySystem->setPageAccess(0x0580 >> mySystem->pageShift(), access);
  mySystem->setPageAccess(0x0680 >> mySystem->pageShift(), access);
  mySystem->setPageAccess(0x0880 >> mySystem->pageShift(), access);
  mySystem->setPageAccess(0x0980 >> mySystem->pageShift(), access);
  mySystem->setPageAccess(0x0C80 >> mySystem->pageShift(), access);
  mySystem->setPageAccess(0x0D80 >> mySystem->pageShift(), access);

  myReadOffset = myWriteOffset = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeGL::checkSwitchBank(uint16_t address, uint8_t)
{
  int slice = -1;
  bool control = false;

  switch(address & 0x1F80)
  {
    case 0x0480: slice = 0; break;
    case 0x0580: slice = 1; break;
    case 0x0880: slice = 2; break;
    case 0x0980: slice = 3; break;
    case 0x0C80: control = true; break;
    default: break;
  }

  if(slice >= 0)
  {
    // bits 0..3 select the bank (0..3 = ROM, 4..15 = RAM)
    bank(address & 0x0F, (uint16_t)slice);
    return true;
  }
  if(control)
  {
    myEnablePROM = (address & 0x30) == 0x30;
    if(myEnablePROM)
    {
      System::PageAccess access(0, 0, 0, this, System::PA_READ);
      mySystem->setPageAccess(0x1FC0 >> mySystem->pageShift(), access);
    }
    else
      mySystem->setPageAccess(0x1FC0 >> mySystem->pageShift(), myOrgAccess);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeGL::peek(uint16_t address)
{
  if(myEnablePROM && ((address & ADDR_MASK) >= 0x1FC0) &&
     ((address & ADDR_MASK) <= 0x1FDF))
    return 0;  // sufficient for the PROM presence check

  checkSwitchBank(address & ADDR_MASK, 0);

  // Reads that reach here are open-bus; use the data-bus value
  return mySystem->getDataBusState(0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeGL::poke(uint16_t address, uint8_t value)
{
  checkSwitchBank(address & ADDR_MASK, value);
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeGL::save(Serializer& out) const
{
  CartridgeEnhanced::save(out);
  out.putBool(myEnablePROM);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeGL::load(Serializer& in)
{
  CartridgeEnhanced::load(in);
  myEnablePROM = in.getBool();
  return true;
}
