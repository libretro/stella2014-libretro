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
#include "CartWD.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const CartridgeWD::BankOrg CartridgeWD::ourBankOrg[8] = {
  { 0, 0, 1, 3 },  // Bank 0
  { 0, 1, 2, 3 },  // Bank 1
  { 4, 5, 6, 7 },  // Bank 2
  { 7, 4, 2, 3 },  // Bank 3
  { 0, 0, 6, 7 },  // Bank 4
  { 0, 1, 7, 6 },  // Bank 5
  { 2, 3, 4, 5 },  // Bank 6
  { 6, 0, 5, 1 }   // Bank 7
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::CartridgeWD(const uint8_t* image, uint32_t size,
                         const Settings& settings)
  : CartridgeEnhanced(image, size, settings, 8192),
    myCyclesAtBankswitchInit(0),
    myPendingBank(0xF0),
    myCurrentBank(0)
{
  // A 8195-byte image has segments 2 and 3 swapped in the file
  if(size == 8195)
  {
    // myImage was filled by the base ctor from the raw image; redo the two
    // swapped 1K segments
    uint8_t tmp[1024];
    memcpy(tmp,               myImage + 1024 * 2, 1024);
    memcpy(myImage + 1024*2,  myImage + 1024 * 3, 1024);
    memcpy(myImage + 1024*3,  tmp,                1024);
  }
  myDirectPeek = false;
  myBankShift  = BANK_SHIFT_WD;
  myRamSize    = RAM_SIZE_WD;
  myRamWpHigh  = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::~CartridgeWD()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::reset()
{
  CartridgeEnhanced::reset();
  myCyclesAtBankswitchInit = 0;
  myPendingBank = 0xF0;
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::install(System& system)
{
  CartridgeEnhanced::install(system);

  uint16_t shift = mySystem->pageShift();
  myHotSpotPageAccess = mySystem->getPageAccess(0x0030 >> shift);

  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint32_t addr = 0x0000; addr < 0x0040; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);

  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::bank(uint16_t bank, uint16_t)
{
  if(bankLocked()) return false;

  myCurrentBank = bank % 8;

  // Arrange the four 1K segments per the fixed table
  CartridgeEnhanced::bank(ourBankOrg[myCurrentBank].zero,  0);
  CartridgeEnhanced::bank(ourBankOrg[myCurrentBank].one,   1);
  CartridgeEnhanced::bank(ourBankOrg[myCurrentBank].two,   2);
  CartridgeEnhanced::bank(ourBankOrg[myCurrentBank].three, 3);

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeWD::peek(uint16_t address)
{
  // Apply a pending bankswitch a few cycles after it was requested
  if(myPendingBank != 0xF0 && !bankLocked() &&
     mySystem->cycles() > (myCyclesAtBankswitchInit + 3))
  {
    bank(myPendingBank);
    myPendingBank = 0xF0;
  }

  if(!(address & 0x1000))
  {
    // A read in $0030-$003F initiates a (delayed) bankswitch
    if(!bankLocked() && (address & 0x00FF) >= 0x30 && (address & 0x00FF) <= 0x3F)
    {
      myCyclesAtBankswitchInit = mySystem->cycles();
      myPendingBank = address & 0x000F;
    }
    return myHotSpotPageAccess.device->peek(address);
  }

  return CartridgeEnhanced::peek(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::poke(uint16_t address, uint8_t value)
{
  if(!(address & 0x1000))
  {
    myHotSpotPageAccess.device->poke(address, value);
    return false;
  }
  return CartridgeEnhanced::poke(address, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::save(Serializer& out) const
{
  CartridgeEnhanced::save(out);
  out.putShort(myCurrentBank);
  out.putInt(myCyclesAtBankswitchInit);
  out.putShort(myPendingBank);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::load(Serializer& in)
{
  CartridgeEnhanced::load(in);
  myCurrentBank = in.getShort();
  myCyclesAtBankswitchInit = in.getInt();
  myPendingBank = in.getShort();
  return true;
}
