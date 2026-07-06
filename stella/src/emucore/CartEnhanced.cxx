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
#include "CartEnhanced.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEnhanced::CartridgeEnhanced(const uint8_t* image, uint32_t size,
                                     const Settings& settings, uint32_t bsSize)
  : Cartridge(settings),
    myBankShift(12),
    myBankSize(0),
    myBankMask(0),
    myRamBankShift(0),
    myRamSize(0),
    myRamBankCount(0),
    myRamMask(0),
    myBankSegs(1),
    myRomOffset(0),
    myWriteOffset(0),
    myReadOffset(0),
    myRamWpHigh(false),
    myDirectPeek(true),
    mySize(bsSize),
    myCurrentSegOffset(0),
    myRAM(0)
{
  // Allocate the ROM image, zero-filled so short ROMs are padded
  myImage = new uint8_t[mySize];
  memset(myImage, 0, mySize);

  // Copy up to the amount of data the ROM provides
  memcpy(myImage, image, (size < mySize) ? size : mySize);

  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEnhanced::~CartridgeEnhanced()
{
  delete[] myImage;
  delete[] myRAM;
  delete[] myCurrentSegOffset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeEnhanced::calcNumSegments() const
{
  // Either the bankswitching supports multiple segments, or the ROM is < 4K
  int bySegment = 1 << (12 - myBankShift);
  int bySize    = (int)(mySize / myBankSize);
  return (uint16_t)((bySegment < bySize) ? bySegment : bySize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhanced::install(System& system)
{
  mySystem = &system;

  if(!myRamBankShift)
    myRamBankShift = myBankShift - 1;

  // Limit banked RAM size to the size of one RAM bank
  uint16_t ramSize = myRamBankCount > 0 ? (uint16_t)(1 << myRamBankShift)
                                        : (uint16_t)myRamSize;

  myBankSize = (uint16_t)(1 << myBankShift);
  myBankMask = (uint16_t)(myBankSize - 1);
  myBankSegs = calcNumSegments();
  // ROM has an offset if RAM lives inside a bank (e.g. for F8SC-style RAM)
  myRomOffset = myRamBankCount > 0 ? 0 : (uint16_t)(myRamSize * 2);
  myRamMask = (uint16_t)(ramSize - 1);
  myWriteOffset = myRamWpHigh ? ramSize : 0;
  myReadOffset  = myRamWpHigh ? 0 : ramSize;

  // Allocate more space only if RAM has its own bank(s)
  createCodeAccessBase(mySize + (myRomOffset > 0 ? 0 : myRamSize));

  // Allocate the array holding each segment's current bank offset
  myCurrentSegOffset = new uint32_t[myBankSegs];
  memset(myCurrentSegOffset, 0, myBankSegs * sizeof(uint32_t));

  // Allocate the RAM area
  if(myRamSize > 0)
  {
    myRAM = new uint8_t[myRamSize];
    memset(myRAM, 0, myRamSize);
  }

  uint16_t shift = mySystem->pageShift();

  if(myRomOffset > 0)
  {
    // Setup page access for extra (non-banked) RAM; banked RAM is set in bank()
    System::PageAccess access(0, 0, 0, this, System::PA_READ);

    // Write port: writes route through poke() to detect read-from-write-port
    access.type = System::PA_WRITE;
    for(uint32_t addr = ROM_OFFSET + myWriteOffset;
        addr < (uint32_t)(ROM_OFFSET + myWriteOffset + myRamSize);
        addr += (1 << shift))
    {
      uint16_t offset = (uint16_t)(addr & myRamMask);
      access.codeAccessBase = &myCodeAccessBase[myWriteOffset + offset];
      mySystem->setPageAccess(addr >> shift, access);
    }

    // Read port: direct peek from RAM
    access.type = System::PA_READ;
    for(uint32_t addr = ROM_OFFSET + myReadOffset;
        addr < (uint32_t)(ROM_OFFSET + myReadOffset + myRamSize);
        addr += (1 << shift))
    {
      uint16_t offset = (uint16_t)(addr & myRamMask);
      access.directPeekBase = &myRAM[offset];
      access.codeAccessBase = &myCodeAccessBase[myReadOffset + offset];
      mySystem->setPageAccess(addr >> shift, access);
    }
  }

  // Install pages for the startup bank (first segment)
  bank(myStartBank, 0);
  if(mySize >= 4096 && myBankSegs > 1)
    // Point the last segment at the last ROM bank
    bank(romBankCount() - 1, myBankSegs - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhanced::reset()
{
  if(myRamSize > 0)
  {
    for(uint32_t i = 0; i < myRamSize; ++i)
      myRAM[i] = mySystem->randGenerator().next();
  }

  myStartBank = getStartBank();
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::bank(uint16_t bank)
{
  return this->bank(bank, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::bank(uint16_t bank, uint16_t segment)
{
  if(bankLocked()) return false;

  uint16_t shift = mySystem->pageShift();
  uint16_t pageMask = (uint16_t)((1 << shift) - 1);
  uint16_t segmentOffset = (uint16_t)(segment << myBankShift);

  if(myRamBankCount == 0 || bank < romBankCount())
  {
    // Setup a ROM bank
    uint16_t romBank = (uint16_t)(bank % romBankCount());
    uint32_t bankOffset = myCurrentSegOffset[segment] = (uint32_t)romBank << myBankShift;

    uint16_t fromAddr = (uint16_t)((ROM_OFFSET + segmentOffset +
                          (segment == 0 ? myRomOffset : 0)) & ~pageMask);
    uint16_t toAddr   = (uint16_t)((ROM_OFFSET + segmentOffset +
                          (mySize < 4096 ? 4096 : myBankSize)) & ~pageMask);

    System::PageAccess access(0, 0, 0, this, System::PA_READ);
    for(uint16_t addr = fromAddr; addr < toAddr; addr += (1 << shift))
    {
      uint32_t offset = bankOffset + (addr & myBankMask);
      if(myDirectPeek)
        access.directPeekBase = &myImage[offset];
      else
        access.directPeekBase = 0;
      access.codeAccessBase = &myCodeAccessBase[offset];
      mySystem->setPageAccess(addr >> shift, access);
    }
  }
  else
  {
    // Setup a RAM bank
    uint16_t ramBank = (uint16_t)((bank - romBankCount()) % myRamBankCount);
    uint32_t bankOffset = mySize + ((uint32_t)ramBank << myRamBankShift);

    // Remember what bank is in this segment (in ROM-bank units)
    myCurrentSegOffset[segment] = mySize + ((uint32_t)ramBank << myBankShift);

    uint16_t span = (uint16_t)(myBankSize >> (myBankShift - myRamBankShift));

    // Write port
    uint16_t fromAddr = (uint16_t)((ROM_OFFSET + segmentOffset + myWriteOffset) & ~pageMask);
    uint16_t toAddr   = (uint16_t)((ROM_OFFSET + segmentOffset + myWriteOffset + span) & ~pageMask);
    System::PageAccess access(0, 0, 0, this, System::PA_WRITE);
    for(uint16_t addr = fromAddr; addr < toAddr; addr += (1 << shift))
    {
      uint32_t offset = bankOffset + (addr & myRamMask);
      access.codeAccessBase = &myCodeAccessBase[offset];
      mySystem->setPageAccess(addr >> shift, access);
    }

    // Read port
    fromAddr = (uint16_t)((ROM_OFFSET + segmentOffset + myReadOffset) & ~pageMask);
    toAddr   = (uint16_t)((ROM_OFFSET + segmentOffset + myReadOffset + span) & ~pageMask);
    access.type = System::PA_READ;
    for(uint16_t addr = fromAddr; addr < toAddr; addr += (1 << shift))
    {
      uint32_t offset = bankOffset + (addr & myRamMask);
      access.directPeekBase = &myRAM[offset - mySize];
      access.codeAccessBase = &myCodeAccessBase[offset];
      mySystem->setPageAccess(addr >> shift, access);
    }
  }

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeEnhanced::bank() const
{
  return (uint16_t)(myCurrentSegOffset[0] >> myBankShift);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeEnhanced::bankCount() const
{
  return romBankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeEnhanced::romBankCount() const
{
  return (uint16_t)(mySize >> myBankShift);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeEnhanced::ramBankCount() const
{
  return myRamBankCount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::isRamBank(uint16_t address) const
{
  if(myRamBankCount == 0) return false;
  uint32_t segOffset = myCurrentSegOffset[((address & ROM_MASK) >> myBankShift) % myBankSegs];
  return segOffset >= mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeEnhanced::peek(uint16_t address)
{
  uint16_t peekAddress = address;

  checkSwitchBank(address & ADDR_MASK, 0);

  if(isRamBank(address))
  {
    address &= myRamMask;
    // A read of the RAM write port. As on the old 2014 carts, this returns
    // the current data-bus value and writes it into the RAM cell (the
    // "read from write port triggers an unwanted write" behaviour). Skipped
    // during ROM autodetection and when the bank is locked.
    uint8_t value = mySystem->getDataBusState(0xFF);
    uint32_t idx = ramAddressSegmentOffset(peekAddress) + address;
    if(!bankLocked() && !mySystem->autodetectMode())
      myRAM[idx] = value;
    return bankLocked() ? myRAM[idx] : value;
  }

  address &= ROM_MASK;

  // Extra-RAM write port read
  if(myRamSize > 0 && address >= myWriteOffset &&
     address < myWriteOffset + myRamSize)
  {
    uint8_t value = mySystem->getDataBusState(0xFF);
    uint32_t idx = address - myWriteOffset;
    if(!bankLocked() && !mySystem->autodetectMode())
      myRAM[idx] = value;
    return bankLocked() ? myRAM[idx] : value;
  }

  return myImage[romAddressSegmentOffset(peekAddress) + (peekAddress & myBankMask)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::poke(uint16_t address, uint8_t value)
{
  // Switch banks if necessary
  if(checkSwitchBank(address & ADDR_MASK, value))
    return false;

  if(myRamSize > 0)
  {
    uint16_t pokeAddress = address;

    if(isRamBank(address))
    {
      if((bool)(address & (myBankSize >> 1)) == myRamWpHigh ||
         myBankShift == myRamBankShift)
      {
        address &= myRamMask;
        myRAM[ramAddressSegmentOffset(pokeAddress) + address] = value;
        return true;
      }
    }
    else
    {
      if((bool)(address & myRamSize) == myRamWpHigh)
      {
        myRAM[address & myRamMask] = value;
        return true;
      }
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::patch(uint16_t address, uint8_t value)
{
  address &= ROM_MASK;

  if(isRamBank(address))
  {
    myRAM[ramAddressSegmentOffset(address) + (address & myRamMask)] = value;
  }
  else
  {
    if(myRamSize > 0 && address >= myReadOffset &&
       address < myReadOffset + myRamSize)
      myRAM[address - myReadOffset] = value;
    else
      myImage[romAddressSegmentOffset(address) + (address & myBankMask)] = value;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeEnhanced::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putInt(myBankSegs);
    for(uint16_t i = 0; i < myBankSegs; ++i)
      out.putInt(myCurrentSegOffset[i]);
    if(myRamSize > 0)
      out.putByteArray(myRAM, myRamSize);
  }
  catch(...)
  {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeEnhanced::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    uint16_t segs = (uint16_t)in.getInt();
    for(uint16_t i = 0; i < segs && i < myBankSegs; ++i)
      myCurrentSegOffset[i] = in.getInt();
    if(myRamSize > 0)
      in.getByteArray(myRAM, myRamSize);
  }
  catch(...)
  {
    return false;
  }

  // Re-map every segment from the restored offsets
  for(uint16_t seg = 0; seg < myBankSegs; ++seg)
  {
    uint32_t off = myCurrentSegOffset[seg];
    uint16_t b;
    if(off >= mySize)
      b = (uint16_t)(romBankCount() + ((off - mySize) >> myBankShift));
    else
      b = (uint16_t)(off >> myBankShift);
    bank(b, seg);
  }

  return true;
}
