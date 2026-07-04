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
#include "TIA.hxx"
#include "CartWD.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Each entry lists the 1K bank mapped into segments 0,1,2,3 respectively.
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
  : Cartridge(settings),
    mySize(size),
    myCurrentBank(0),
    myCyclesAtBankswitchInit(0),
    myPendingBank(0xF0)
{
  // Allocate array for the ROM image (always 8K for this scheme)
  myImage = new uint8_t[8192];
  memset(myImage, 0, 8192);

  if(size == 8195)
  {
    // Bad-dump variant: correct size is 8K, with banks 2 & 3 swapped
    memcpy(myImage, image, 1024 * 2);
    memcpy(myImage + 1024 * 2, image + 1024 * 3, 1024);
    memcpy(myImage + 1024 * 3, image + 1024 * 2, 1024);
    memcpy(myImage + 1024 * 4, image + 1024 * 4, 1024 * 4);
    mySize = 8192;
  }
  else
  {
    memcpy(myImage, image, size < 8192 ? size : 8192);
    mySize = 8192;
  }

  createCodeAccessBase(8192 + 64);

  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeWD::~CartridgeWD()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::reset()
{
  for(uint32_t i = 0; i < 64; ++i)
    myRAM[i] = mySystem->randGenerator().next();

  myCyclesAtBankswitchInit = 0;
  myPendingBank = 0xF0;

  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();

  // Save the page access for the hotspot region so we can forward TIA
  // accesses through it.
  myHotSpotPageAccess = mySystem->getPageAccess(0x0030 >> shift);

  // The hotspots are in TIA address space ($0030-$003F); claim those pages
  // so hotspot reads come through peek().
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  for(uint32_t addr = 0x0000; addr < 0x0040; addr += (1 << shift))
    mySystem->setPageAccess(addr >> shift, access);

  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeWD::segment(uint16_t bank, uint16_t seg)
{
  // Record which 1K ROM bank is mapped into this segment. The actual
  // reads are serviced through peek() (intercept model) rather than direct
  // page access, so that the delayed bankswitch timing check runs on every
  // ROM read -- matching the hardware, where the switch takes effect a few
  // cycles after the triggering hotspot access.
  mySegmentBank[seg] = bank;

  uint16_t shift = mySystem->pageShift();
  uint16_t base = (uint16_t)(0x1000 + (seg << 10));

  System::PageAccess access(0, 0, 0, this, System::PA_READ);

  uint32_t start = base;

  // Segment 0's first 128 bytes are the RAM read/write ports, not ROM.
  if(seg == 0)
  {
    // RAM write port: $1040-$107F (direct poke into RAM)
    access.type = System::PA_WRITE;
    access.directPeekBase = 0;
    for(uint32_t w = 0x1040; w < 0x1080; w += (1 << shift))
    {
      access.directPokeBase = &myRAM[w & 0x003F];
      access.codeAccessBase = &myCodeAccessBase[8192 + (w & 0x003F)];
      mySystem->setPageAccess(w >> shift, access);
    }

    // RAM read port: $1000-$103F (direct peek from RAM)
    access.type = System::PA_READ;
    access.directPokeBase = 0;
    for(uint32_t r = 0x1000; r < 0x1040; r += (1 << shift))
    {
      access.directPeekBase = &myRAM[r & 0x003F];
      access.codeAccessBase = &myCodeAccessBase[8192 + (r & 0x003F)];
      mySystem->setPageAccess(r >> shift, access);
    }

    // ROM for the rest of segment 0 begins at $1080
    start = 0x1080;
  }

  // Map the ROM portion of this segment as intercept pages: null
  // directPeekBase routes every read through peek().
  access.type = System::PA_READ;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  for(uint32_t address = start; address < (uint32_t)(base + 0x0400);
      address += (1 << shift))
  {
    access.codeAccessBase = &myCodeAccessBase[((uint32_t)bank << 10) + (address & 0x03FF)];
    mySystem->setPageAccess(address >> shift, access);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::bank(uint16_t bank)
{
  if(bankLocked()) return false;

  myCurrentBank = bank % 8;

  segment(ourBankOrg[myCurrentBank].zero,  0);
  segment(ourBankOrg[myCurrentBank].one,   1);
  segment(ourBankOrg[myCurrentBank].two,   2);
  segment(ourBankOrg[myCurrentBank].three, 3);

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeWD::peek(uint16_t address)
{
  // Is it time to commit a pending bankswitch? The hardware applies the
  // switch a few cycles after the triggering hotspot read. This check runs
  // on every read (ROM or hotspot), matching the hardware timing.
  if(myPendingBank != 0xF0 && !bankLocked() &&
     mySystem->cycles() > (myCyclesAtBankswitchInit + 3))
  {
    bank(myPendingBank);
    myPendingBank = 0xF0;
  }

  if(!(address & 0x1000))
  {
    // Hotspots at $0030-$003F in TIA address space
    if(!bankLocked() && (address & 0x00FF) >= 0x30 && (address & 0x00FF) <= 0x3F)
    {
      myCyclesAtBankswitchInit = mySystem->cycles();
      myPendingBank = address & 0x000F;
    }
    // Forward the (TIA-space) read to the underlying device
    return myHotSpotPageAccess.device->peek(address);
  }

  // A ROM read (segment intercept). Return the byte from the 1K bank that
  // is currently mapped into the addressed segment.
  uint16_t a = address & 0x0FFF;
  int seg = a >> 10;
  uint32_t offset = (uint32_t)mySegmentBank[seg] << 10;
  return myImage[offset + (a & 0x03FF)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::poke(uint16_t address, uint8_t value)
{
  // Writes below $0040 are TIA writes
  if(!(address & 0x1000))
    myHotSpotPageAccess.device->poke(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeWD::bank() const
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeWD::bankCount() const
{
  return 8;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::patch(uint16_t address, uint8_t value)
{
  address &= 0x0FFF;

  if(address < 0x0040)
  {
    // Patch the RAM (read port)
    myRAM[address & 0x003F] = value;
  }
  else
  {
    int seg = address >> 10;
    uint32_t offset = (uint32_t)mySegmentBank[seg] << 10;
    myImage[offset + (address & 0x03FF)] = value;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeWD::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putShort(myCurrentBank);
    out.putByteArray(myRAM, 64);
    out.putInt(myCyclesAtBankswitchInit);
    out.putShort(myPendingBank);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeWD::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCurrentBank = in.getShort();
    in.getByteArray(myRAM, 64);
    myCyclesAtBankswitchInit = in.getInt();
    myPendingBank = in.getShort();

    bank(myCurrentBank);
  }
  catch(...)
  {
    return false;
  }

  return true;
}
