//============================================================================
// CartBUS for stella2014 - backported from Stella 7, adapted to the 2014
// Cartridge API and the C Thumbulator. Integer music clock (no double).
//============================================================================

#include <cstring>

#include "System.hxx"
#include "TIA.hxx"
#include "M6532.hxx"
#include "Serializer.hxx"
#include "Settings.hxx"
#include "CartBUS.hxx"
#ifdef THUMB_SUPPORT
#include "Thumbulator.h"
#endif

namespace {
  inline bool BUS_STUFF_ON(uint8_t mode)     { return (mode & 0x0F) == 0; }
  inline bool DIGITAL_AUDIO_ON(uint8_t mode) { return (mode & 0xF0) == 0; }

  inline uint32_t getUInt32(const uint8_t* a, uint32_t addr) {
    return uint32_t(a[addr + 0])       +
           uint32_t(a[addr + 1] << 8)  +
           uint32_t(a[addr + 2] << 16) +
           uint32_t(a[addr + 3] << 24);
  }

  const uint8_t COMMSTREAM = 0x10;
  const uint8_t JUMPSTREAM = 0x11;
}

/* extern "C" trampoline so the C Thumbulator can call back into this cart. */
extern "C" uint32_t bus_thumb_callback(void* cart, uint8_t fn,
                                       uint32_t v1, uint32_t v2)
{
  return static_cast<CartridgeBUS*>(cart)->thumbCallback(fn, v1, v2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBUS::CartridgeBUS(const uint8_t* image, uint32_t size,
                           const Settings& settings)
  : CartridgeARM(settings),
    myImage(0),
    mySize(32u * 1024u),
    myProgramImage(0),
    myDisplayImage(0),
    myDriverImage(0),
    myBankOffset(0),
    myCurrentBank(0),
    myBusOverdriveAddress(0),
    mySTYZeroPageAddress(0),
    myJMPoperandAddress(0),
    myAudioCycles(0),
    myARMCycles(0),
    myDatastreamBase(0),
    myDatastreamIncrementBase(0),
    myDatastreamMapBase(0),
    myWaveformBase(0),
    myFractionalClocksFrac(0),
    myClockRate(715909u),
    myMode(0),
    myFastJumpActive(0),
    myBUSSubtype(BUSSubtype_BUS1)
{
  int i;

  myImage = new uint8_t[32 * 1024];
  memset(myImage, 0, 32 * 1024);
  memcpy(myImage, image, size < 32u * 1024u ? size : 32u * 1024u);

  for(i = 0; i < 3; ++i)
  {
    myMusicCounters[i] = 0;
    myMusicFrequencies[i] = 0;
    myMusicWaveformSize[i] = 27;
  }
  memset(myRAM, 0, sizeof(myRAM));

  setupVersion();

  myDriverImage = myRAM;

  {
    uint32_t cBase, cStart, cStack;
    if(myBUSSubtype == BUSSubtype_BUS0)
    {
      myProgramImage = myImage + 3 * 1024;
      myDisplayImage = myRAM + 0x0C00;
      cBase = 0x00000C00; cStart = 0x00000C08; cStack = 0x40001FFC;
    }
    else
    {
      myProgramImage = myImage + 4 * 1024;
      myDisplayImage = myRAM + 0x0800;
      cBase = 0x00000800; cStart = 0x00000808; cStack = 0x40001FFC;
    }

#ifdef THUMB_SUPPORT
    createThumbulator((const uint16_t*)myImage, (uint16_t*)myRAM);
    thumb_init_ex(myThumbEmulator, (const uint16_t*)myImage,
                  (uint16_t*)myRAM,
                  mySettings.getBool("thumb.trapfatal"),
                  cStart, cStack, cBase);
    thumb_set_callback(myThumbEmulator, THUMB_CONFIG_BUS, this,
                       bus_thumb_callback);
#else
    (void)cBase; (void)cStart; (void)cStack;  /* only used with THUMB_SUPPORT */
#endif
  }

  myStartBank = (myBUSSubtype == BUSSubtype_BUS0) ? 5 : 6;
  setInitialState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBUS::~CartridgeBUS()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::reset()
{
  int i, start;

  /* RAM above the driver is randomized/zeroed on power-up. */
  start = (myBUSSubtype == BUSSubtype_BUS0) ? 3 * 1024 : 2 * 1024;
  if(mySettings.getBool("ramrandom"))
    for(i = start; i < (int)sizeof(myRAM); ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    for(i = start; i < (int)sizeof(myRAM); ++i)
      myRAM[i] = 0;

  myAudioCycles = 0;
  myARMCycles = 0;
  myFractionalClocksFrac = 0;

  setInitialState();
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setInitialState()
{
  int i;

  if(myBUSSubtype == BUSSubtype_BUS0)
    memcpy(myDriverImage, myImage, 3 * 1024);
  else
    memcpy(myDriverImage, myImage, 2 * 1024);

  for(i = 0; i < 3; ++i)
    myMusicWaveformSize[i] = 27;

  myMode = 0xFF;
  myBankOffset = 0;
  myBusOverdriveAddress = 0;
  mySTYZeroPageAddress = 0;
  myJMPoperandAddress = 0;
  myFastJumpActive = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  uint32_t addr;

  for(addr = 0x1000; addr < 0x2000; addr += (1u << shift))
    mySystem->setPageAccess(addr >> shift, access);

  /* Take over the TIA and RIOT address space so bus-stuffing (overdrive)
     can intercept those accesses, as Cart4A50 does. */
  mySystem->tia().install(system, *this);
  mySystem->m6532().install(system, *this);

  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::bank(uint16_t bank)
{
  if(bankLocked()) return false;

  myCurrentBank = bank;
  myBankOffset = bank << 12;
  uint16_t shift = mySystem->pageShift();
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  uint32_t address;
  for(address = 0x1040; address < 0x2000; address += (1u << shift))
  {
    access.codeAccessBase = &myCodeAccessBase[myBankOffset + (address & 0x0FFF)];
    mySystem->setPageAccess(address >> shift, access);
  }
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeBUS::bank() const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeBUS::bankCount() const
{
  return 7;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::patch(uint16_t address, uint8_t value)
{
  address &= 0x0FFF;
  myProgramImage[myBankOffset + address] = value;
  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeBUS::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::thumbCallback(uint8_t function, uint32_t value1,
                                     uint32_t value2)
{
  switch(function)
  {
    case 0: myMusicFrequencies[value1] = value2; break;
    case 1: myMusicCounters[value1] = 0; break;
    case 2: return myMusicCounters[value1];
    case 3: myMusicWaveformSize[value1] = value2; break;
    default: break;
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::callFunction(uint8_t value)
{
#ifdef THUMB_SUPPORT
  switch(value)
  {
    case 254:
    case 255:
    {
      uint32_t cycles = (uint32_t)(mySystem->cycles() - myARMCycles);
      myARMCycles = mySystem->cycles();
      runArm(cycles, value == 254);
      updateCycles(cycles);
      break;
    }
    default:
      break;
  }
#else
  (void)value;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::updateMusicModeDataFetchers()
{
  uint32_t cycles = (uint32_t)(mySystem->cycles() - myAudioCycles);
  uint32_t num = (myClockRate == 715909u) ? 12000u : 10000u;
  uint64_t scaled;
  uint32_t whole;
  int x;

  myAudioCycles = mySystem->cycles();

  scaled = (uint64_t)num * cycles + myFractionalClocksFrac;
  whole = (uint32_t)(scaled / myClockRate);
  myFractionalClocksFrac = (uint32_t)(scaled % myClockRate);

  if(whole > 0)
    for(x = 0; x <= 2; ++x)
      myMusicCounters[x] += myMusicFrequencies[x] * whole;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeBUS::busOverdrive(uint16_t address)
{
  uint8_t overdrive = 0xff;

  if(address == myBusOverdriveAddress)
  {
    uint8_t map = address & 0x7f;
    if(map <= 0x24)  /* TIA registers VSYNC..HMBL inclusive */
    {
      uint32_t alldatastreams = getAddressMap(map);
      uint8_t datastream = alldatastreams & 0x0f;
      overdrive = readFromDatastream(datastream);

      alldatastreams >>= 4;
      alldatastreams |= (datastream << 28);
      setAddressMap(map, alldatastreams);
    }
  }

  myBusOverdriveAddress = 0xff;  /* one-shot */
  return overdrive;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::getDatastreamPointer(uint8_t index) const
{
  uint16_t address = myDatastreamBase + index * 4;
  return  myRAM[address + 0]        + (myRAM[address + 1] << 8)
        + (myRAM[address + 2] << 16) + (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setDatastreamPointer(uint8_t index, uint32_t value)
{
  uint16_t address = myDatastreamBase + index * 4;
  myRAM[address + 0] = value & 0xff;
  myRAM[address + 1] = (value >> 8) & 0xff;
  myRAM[address + 2] = (value >> 16) & 0xff;
  myRAM[address + 3] = (value >> 24) & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::getDatastreamIncrement(uint8_t index) const
{
  uint16_t address = myDatastreamIncrementBase + index * 4;
  return  myRAM[address + 0]        + (myRAM[address + 1] << 8)
        + (myRAM[address + 2] << 16) + (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setDatastreamIncrement(uint8_t index, uint32_t value)
{
  uint16_t address = myDatastreamIncrementBase + index * 4;
  myRAM[address + 0] = value & 0xff;
  myRAM[address + 1] = (value >> 8) & 0xff;
  myRAM[address + 2] = (value >> 16) & 0xff;
  myRAM[address + 3] = (value >> 24) & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::getAddressMap(uint8_t index) const
{
  uint16_t address = myDatastreamMapBase + index * 4;
  return  myRAM[address + 0]        + (myRAM[address + 1] << 8)
        + (myRAM[address + 2] << 16) + (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setAddressMap(uint8_t index, uint32_t value)
{
  uint16_t address = myDatastreamMapBase + index * 4;
  myRAM[address + 0] = value & 0xff;
  myRAM[address + 1] = (value >> 8) & 0xff;
  myRAM[address + 2] = (value >> 16) & 0xff;
  myRAM[address + 3] = (value >> 24) & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::getWaveform(uint8_t index) const
{
  uint16_t address = myWaveformBase + index * 4;
  uint32_t result = myRAM[address + 0]        + (myRAM[address + 1] << 8)
                  + (myRAM[address + 2] << 16) + (myRAM[address + 3] << 24);
  result -= 0x40000800;
  if(result >= 4096)
    result = 0;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::getSample()
{
  uint16_t address = myWaveformBase;
  return  myRAM[address + 0]        + (myRAM[address + 1] << 8)
        + (myRAM[address + 2] << 16) + (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::getWaveformSize(uint8_t index) const
{
  return myMusicWaveformSize[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeBUS::readFromDatastream(uint8_t index)
{
  uint32_t pointer = getDatastreamPointer(index);
  uint16_t increment = getDatastreamIncrement(index);
  uint8_t value = myDisplayImage[pointer >> 20];
  pointer += (increment << 12);
  setDatastreamPointer(index, pointer);
  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeBUS::peek(uint16_t address)
{
  /* Hotspots below 0x1000: route to RIOT/TIA (we took over that space). */
  if(!(address & 0x1000))
  {
    uint16_t lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      return mySystem->m6532().peek(address);
    else if(!(lowAddress & 0x200))
      return mySystem->tia().peek(address);
    return 0;
  }

  {
    uint8_t peekvalue;
    address &= 0x0FFF;
    peekvalue = myProgramImage[myBankOffset + address];

    if(bankLocked())
      return peekvalue;

    if(myBUSSubtype == BUSSubtype_BUS3)
    {
      if(myFastJumpActive && myJMPoperandAddress == address)
      {
        uint32_t pointer;
        uint8_t value;
        --myFastJumpActive;
        ++myJMPoperandAddress;
        pointer = getDatastreamPointer(JUMPSTREAM);
        value = myDisplayImage[pointer >> 20];
        pointer += 0x100000;
        setDatastreamPointer(JUMPSTREAM, pointer);
        return value;
      }

      if(BUS_STUFF_ON(myMode)
         && peekvalue == 0x4C
         && myProgramImage[myBankOffset + address + 1] == 0
         && myProgramImage[myBankOffset + address + 2] == 0)
      {
        myFastJumpActive = 2;
        myJMPoperandAddress = address + 1;
        return peekvalue;
      }
      myJMPoperandAddress = 0;
    }

    if(BUS_STUFF_ON(myMode) && mySTYZeroPageAddress == address)
      myBusOverdriveAddress = peekvalue;
    mySTYZeroPageAddress = 0;

    if(address < 0x20 &&
       (myBUSSubtype == BUSSubtype_BUS1 || myBUSSubtype == BUSSubtype_BUS2))
    {
      uint8_t result = 0;
      uint32_t index = address & 0x0f;
      uint32_t function = (address >> 4) & 0x01;

      if(function == 0x00)
      {
        result = readFromDatastream(index);
      }
      else
      {
        if(index == 0x08)  /* AMPLITUDE */
        {
          uint32_t i;
          updateMusicModeDataFetchers();
          i = myDisplayImage[getWaveform(0) + (myMusicCounters[0] >> myMusicWaveformSize[0])]
            + myDisplayImage[getWaveform(1) + (myMusicCounters[1] >> myMusicWaveformSize[1])]
            + myDisplayImage[getWaveform(2) + (myMusicCounters[2] >> myMusicWaveformSize[2])];
          result = (uint8_t)i;
        }
      }
      return result;
    }
    else if(address >= 0xFEE && address <= 0xFF3 &&
            myBUSSubtype == BUSSubtype_BUS3)
    {
      if(address == 0xFEE)  /* AMPLITUDE */
      {
        updateMusicModeDataFetchers();
        if(DIGITAL_AUDIO_ON(myMode))
        {
          uint32_t sampleaddress = getSample() + (myMusicCounters[0] >> 21);
          if(sampleaddress < 0x8000)
            peekvalue = myImage[sampleaddress];
          else if(sampleaddress >= 0x40000000 && sampleaddress < 0x40002000)
            peekvalue = myRAM[sampleaddress - 0x40000000];
          else
            peekvalue = 0;
          if((myMusicCounters[0] & (1 << 20)) == 0)
            peekvalue >>= 4;
          peekvalue &= 0x0f;
        }
        else
        {
          uint32_t i =
              myDisplayImage[getWaveform(0) + (myMusicCounters[0] >> myMusicWaveformSize[0])]
            + myDisplayImage[getWaveform(1) + (myMusicCounters[1] >> myMusicWaveformSize[1])]
            + myDisplayImage[getWaveform(2) + (myMusicCounters[2] >> myMusicWaveformSize[2])];
          peekvalue = (uint8_t)i;
        }
      }
      else if(address == 0xFEF)  /* DSREAD */
        peekvalue = readFromDatastream(COMMSTREAM);
    }
    else if(myBUSSubtype == BUSSubtype_BUS0)
    {
      if(address <= 0x0f)
        peekvalue = readFromDatastream(address);
      else switch(address)
      {
        case 0xFF6: bank(0); break;
        case 0xFF7: bank(1); break;
        case 0xFF8: bank(2); break;
        case 0xFF9: bank(3); break;
        case 0xFFA: bank(4); break;
        case 0xFFB: bank(5); break;
        default: break;
      }
    }
    else
    {
      switch(address)
      {
        case 0xFF5: bank(0); break;
        case 0xFF6: bank(1); break;
        case 0xFF7: bank(2); break;
        case 0xFF8: bank(3); break;
        case 0xFF9: bank(4); break;
        case 0xFFA: bank(5); break;
        case 0xFFB: bank(6); break;
        default: break;
      }
    }

    if(BUS_STUFF_ON(myMode) && peekvalue == 0x84)
      mySTYZeroPageAddress = address + 1;

    return peekvalue;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::poke(uint16_t address, uint8_t value)
{
  if(!(address & 0x1000))
  {
    uint16_t lowAddress;
    value &= busOverdrive(address);
    lowAddress = address & 0x3ff;
    if(lowAddress & 0x80)
      mySystem->m6532().poke(address, value);
    else if(!(lowAddress & 0x200))
      mySystem->tia().poke(address, value);
    return false;
  }

  address &= 0x0FFF;

  if(myBUSSubtype == BUSSubtype_BUS0)
  {
    uint32_t index = address & 0x0f;
    uint32_t pointer;

    if(address >= 0x10 && address <= 0x13)  /* DSxWRITE */
    {
      pointer = getDatastreamPointer(index);
      myDisplayImage[pointer >> 20] = value;
      pointer += 0x100000;
      setDatastreamPointer(index, pointer);
    }
    else if(address == 0x1B)  /* CALLFN */
      callFunction(value);
    else if(address == 0x1C)  /* BUSSTUFF */
      myMode = (value == 0) ? 0 : 0x0f;
    else if(address >= 0x20 && address <= 0x2f)  /* DSxPTR */
    {
      pointer = getDatastreamPointer(index);
      pointer <<= 8;
      pointer &= 0xf0000000;
      pointer |= (value << 20);
      setDatastreamPointer(index, pointer);
    }
    else if(address >= 0x30 && address <= 0x3f)  /* DSxINC */
    {
      uint32_t increment = getDatastreamIncrement(index);
      index <<= 8;
      index |= value;
      index &= 0xffff;
      setDatastreamIncrement(index, increment);
    }
    else switch(address)
    {
      case 0xFF6: bank(0); break;
      case 0xFF7: bank(1); break;
      case 0xFF8: bank(2); break;
      case 0xFF9: bank(3); break;
      case 0xFFA: bank(4); break;
      case 0xFFB: bank(5); break;
      default: break;
    }
  }
  else if(myBUSSubtype == BUSSubtype_BUS3)
  {
    uint32_t pointer;
    switch(address)
    {
      case 0xFF0:  /* DSWRITE */
        pointer = getDatastreamPointer(COMMSTREAM);
        myDisplayImage[pointer >> 20] = value;
        pointer += 0x100000;
        setDatastreamPointer(COMMSTREAM, pointer);
        break;
      case 0xFF1:  /* DSPTR */
        pointer = getDatastreamPointer(COMMSTREAM);
        pointer <<= 8;
        pointer &= 0xf0000000;
        pointer |= (value << 20);
        setDatastreamPointer(COMMSTREAM, pointer);
        break;
      case 0xFF2: myMode = value; break;      /* SETMODE */
      case 0xFF3: callFunction(value); break; /* CALLFN */
      default: break;
    }
  }
  else if(address >= 0xFF5)  /* BUS1/BUS2 bank hotspots */
  {
    switch(address)
    {
      case 0xFF5: bank(0); break;
      case 0xFF6: bank(1); break;
      case 0xFF7: bank(2); break;
      case 0xFF8: bank(3); break;
      case 0xFF9: bank(4); break;
      case 0xFFA: bank(5); break;
      case 0xFFB: bank(6); break;
      default: break;
    }
  }
  else  /* BUS1/BUS2 registers 0x10-0x1F */
  {
    if(address >= 0x10 && address <= 0x1F)
    {
      uint32_t index = address & 0x0f;
      uint32_t pointer;
      if(index <= 0x03)  /* DSxWRITE */
      {
        pointer = getDatastreamPointer(index);
        myDisplayImage[pointer >> 20] = value;
        pointer += 0x100000;
        setDatastreamPointer(index, pointer);
      }
      else if(index >= 0x04 && index <= 0x07)  /* DSxPTR */
      {
        index &= 0x03;
        pointer = getDatastreamPointer(index);
        pointer <<= 8;
        pointer &= 0xf0000000;
        pointer |= (value << 20);
        setDatastreamPointer(index, pointer);
      }
      else if(index == 0x09)  /* STUFFMODE */
        myMode = (value == 0) ? 0 : 0x0f;
      else if(index == 0x0A)  /* CALLFN */
        callFunction(value);
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeBUS::scanBUSDriver(uint32_t searchValue)
{
  int i;
  for(i = 0; i < 2048; i += 4)
    if(getUInt32(myImage, i) == searchValue)
      return i;
  return 0xFFFFFFFFu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBUS::setupVersion()
{
  uint32_t busOffset = scanBUSDriver(0x00535542);  /* "BUS\0" */

  switch(busOffset)
  {
    case 0x7f4:  /* draconian */
      myBUSSubtype = BUSSubtype_BUS1;
      myDatastreamBase = 0x06E0;
      myDatastreamIncrementBase = 0x0720;
      myDatastreamMapBase = 0x0760;
      myWaveformBase = 0x07F4;
      break;
    case 0x778:  /* 128bus, chronocolour, parrot */
      myBUSSubtype = BUSSubtype_BUS2;
      myDatastreamBase = 0x06E0;
      myDatastreamIncrementBase = 0x0720;
      myDatastreamMapBase = 0x0760;
      myWaveformBase = 0x07F4;
      break;
    case 0x770:  /* rpg */
      myBUSSubtype = BUSSubtype_BUS3;
      myDatastreamBase = 0x06D8;
      myDatastreamIncrementBase = 0x0720;
      myDatastreamMapBase = 0x0760;
      myWaveformBase = 0x07F4;
      break;
    default:     /* BUS0, original 3K driver */
      myBUSSubtype = BUSSubtype_BUS0;
      myDatastreamBase = 0x0AE0;
      myDatastreamIncrementBase = 0x0B20;
      myDatastreamMapBase = 0x0B64;
      myWaveformBase = 0x07F4;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
    out.putByteArray(myRAM, sizeof(myRAM));
    out.putShort(myBusOverdriveAddress);
    out.putShort(mySTYZeroPageAddress);
    out.putShort(myJMPoperandAddress);
    out.putInt(myAudioCycles);
    out.putInt(myFractionalClocksFrac);
    out.putInt(myARMCycles);
    out.putIntArray(myMusicCounters, 3);
    out.putIntArray(myMusicFrequencies, 3);
    out.putByteArray(myMusicWaveformSize, 3);
    out.putByte(myMode);
    out.putByte(myFastJumpActive);
    CartridgeARM::saveArmState(out);
  }
  catch(...)
  {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeBUS::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
    in.getByteArray(myRAM, sizeof(myRAM));
    myBusOverdriveAddress = in.getShort();
    mySTYZeroPageAddress = in.getShort();
    myJMPoperandAddress = in.getShort();
    myAudioCycles = in.getInt();
    myFractionalClocksFrac = in.getInt();
    myARMCycles = in.getInt();
    in.getIntArray(myMusicCounters, 3);
    in.getIntArray(myMusicFrequencies, 3);
    in.getByteArray(myMusicWaveformSize, 3);
    myMode = in.getByte();
    myFastJumpActive = in.getByte();
    CartridgeARM::loadArmState(in);
  }
  catch(...)
  {
    return false;
  }

  bank(myBankOffset >> 12);
  return true;
}
