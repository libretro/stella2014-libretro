//============================================================================
// CartCDF for stella2014 - backported from Stella 7, adapted to the 2014
// Cartridge API and the C Thumbulator. Integer music clock (no double).
//============================================================================

#include <cstring>

#include "System.hxx"
#include "Serializer.hxx"
#include "Settings.hxx"
#include "CartCDF.hxx"
#ifdef THUMB_SUPPORT
#include "Thumbulator.h"
#endif

namespace {
  /* Mode helpers: fast-fetch and digital-audio enable bits. */
  inline bool FAST_FETCH_ON(uint8_t mode)    { return (mode & 0x0F) == 0; }
  inline bool DIGITAL_AUDIO_ON(uint8_t mode) { return (mode & 0xF0) == 0; }

  inline uint32_t getUInt32(const uint8_t* a, uint32_t addr) {
    return uint32_t(a[addr + 0])        +
           uint32_t(a[addr + 1] << 8)   +
           uint32_t(a[addr + 2] << 16)  +
           uint32_t(a[addr + 3] << 24);
  }

  const uint8_t  COMMSTREAM      = 0x20;
  const uint8_t  JUMPSTREAM_BASE = 0x21;
  const uint16_t LDAXY_OVERRIDE_INACTIVE = 0xFFFF;
}

/* extern "C" trampoline so the C Thumbulator can call back into this C++
   cartridge. 'cart' is the CartridgeCDF registered via thumb_set_callback. */
extern "C" uint32_t cdf_thumb_callback(void* cart, uint8_t fn,
                                       uint32_t v1, uint32_t v2)
{
  return static_cast<CartridgeCDF*>(cart)->thumbCallback(fn, v1, v2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::CartridgeCDF(const uint8_t* image, uint32_t size,
                           const Settings& settings)
  : CartridgeARM(settings),
    myImage(0),
    mySize(size < 512u * 1024u ? size : 512u * 1024u),
    myProgramImage(0),
    myDisplayImage(0),
    myDriverImage(0),
    myBankOffset(0),
    myCurrentBank(0),
    myAudioCycles(0),
    myARMCycles(0),
    myFractionalClocksFrac(0),
    myMode(0xFF),
    myLDAXYimmediateOperandAddress(LDAXY_OVERRIDE_INACTIVE),
    myLDXenabled(false),
    myLDYenabled(false),
    myFastFetcherOffset(0),
    myJMPoperandAddress(0),
    myFastJumpActive(0),
    myDatastreamBase(0),
    myDatastreamIncrementBase(0),
    myWaveformBase(0),
    myAmplitudeStream(0),
    myFastjumpStreamIndexMask(0),
    myFastJumpStream(0),
    myClockRate(715909u),   /* NTSC music-clock denominator by default */
    myCDFSubtype(CDFSubtype_CDF0)
{
  int i;

  myImage = new uint8_t[mySize];
  memcpy(myImage, image, mySize);
  for(i = 0; i < 3; ++i)
  {
    myMusicCounters[i] = 0;
    myMusicFrequencies[i] = 0;
    myMusicWaveformSize[i] = 27;
  }
  memset(myRAM, 0, sizeof(myRAM));

  setupVersion();

  myProgramImage = myImage + (isCDFJplus() ? 2 * 1024 : 4 * 1024);
  myDriverImage  = myRAM;
  myDisplayImage = myRAM + 2 * 1024;

  {
    uint32_t cBase, cStart, cStack;
    if(isCDFJplus())
    {
      cBase  = getUInt32(myImage, 0x17F8) & 0xFFFFFFFE;
      cStart = cBase;
      cStack = getUInt32(myImage, 0x17F4);
    }
    else
    {
      cBase  = 0x800;
      cStart = 0x808;
      cStack = 0x40001FFC;
    }

#ifdef THUMB_SUPPORT
    createThumbulator((const uint16_t*)myImage, (uint16_t*)myRAM);
    thumb_init_ex(myThumbEmulator, (const uint16_t*)myImage,
                  (uint16_t*)myRAM,
                  mySettings.getBool("thumb.trapfatal"),
                  cStart, cStack, cBase);
    {
      int cfg = (myCDFSubtype == CDFSubtype_CDF0)
                  ? THUMB_CONFIG_CDF : THUMB_CONFIG_CDF1;
      thumb_set_callback(myThumbEmulator, cfg, this, cdf_thumb_callback);
    }
#else
    (void)cBase; (void)cStart; (void)cStack;  /* only used with THUMB_SUPPORT */
#endif
  }

  myStartBank = isCDFJplus() ? 0 : 6;
  setInitialState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCDF::~CartridgeCDF()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::reset()
{
  int i;

  /* RAM above the 2K driver is randomized/zeroed on power-up. */
  if(mySettings.getBool("ramrandom"))
    for(i = 2 * 1024; i < (int)sizeof(myRAM); ++i)
      myRAM[i] = mySystem->randGenerator().next();
  else
    for(i = 2 * 1024; i < (int)sizeof(myRAM); ++i)
      myRAM[i] = 0;

  myAudioCycles = 0;
  myARMCycles = 0;
  myFractionalClocksFrac = 0;

  setInitialState();
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setInitialState()
{
  int i;

  /* Copy the 2K ARM driver into the driver region of RAM. */
  memcpy(myDriverImage, myImage, 2 * 1024);

  for(i = 0; i < 3; ++i)
    myMusicWaveformSize[i] = 27;

  myMode = 0xFF;
  myBankOffset = 0;
  myJMPoperandAddress = 0;
  myFastJumpActive = 0;
  myFastJumpStream = 0;
  myLDAXYimmediateOperandAddress = LDAXY_OVERRIDE_INACTIVE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::install(System& system)
{
  mySystem = &system;
  uint16_t shift = mySystem->pageShift();
  System::PageAccess access(0, 0, 0, this, System::PA_READ);
  uint32_t i;
  for(i = 0x1000; i < 0x1040; i += (1u << shift))
    mySystem->setPageAccess(i >> shift, access);
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::bank(uint16_t bank)
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
uint16_t CartridgeCDF::bank() const
{
  return myBankOffset >> 12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t CartridgeCDF::bankCount() const
{
  return 7;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::patch(uint16_t address, uint8_t value)
{
  address &= 0x0FFF;
  if(address >= 0x0040)
  {
    myProgramImage[myBankOffset + (address & 0x0FFF)] = value;
    return myBankChanged = true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t* CartridgeCDF::getImage(int& size) const
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::thumbCallback(uint8_t function, uint32_t value1,
                                     uint32_t value2)
{
  switch(function)
  {
    case 0:
      /* myMusicFrequencies[value1] = value2 */
      myMusicFrequencies[value1] = value2;
      break;
    case 1:
      myMusicCounters[value1] = 0;
      break;
    case 2:
      return myMusicCounters[value1];
    case 3:
      myMusicWaveformSize[value1] = value2;
      break;
    default:
      break;
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::callFunction(uint8_t value)
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
void CartridgeCDF::updateMusicModeDataFetchers()
{
  /* Integer music clock: whole = (12000|10000 * cycles + frac) / clockDen,
     matching the DPC music-clock conversion. myClockRate holds the
     denominator; the numerator is 12000 (NTSC) or 10000 (PAL/SECAM),
     which we recover from the denominator selection. To keep this simple
     and exact, both numerator and denominator are stored together: we use
     numerator 12000 with denominator 715909 for NTSC and 10000/591149 for
     PAL/SECAM. myClockRate stores the denominator; the matching numerator
     is derived below. */
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
uint32_t CartridgeCDF::getDatastreamPointer(uint8_t index) const
{
  uint16_t address = myDatastreamBase + index * 4;
  return  myRAM[address + 0]        +
         (myRAM[address + 1] << 8)  +
         (myRAM[address + 2] << 16) +
         (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setDatastreamPointer(uint8_t index, uint32_t value)
{
  uint16_t address = myDatastreamBase + index * 4;
  myRAM[address + 0] = value & 0xff;
  myRAM[address + 1] = (value >> 8) & 0xff;
  myRAM[address + 2] = (value >> 16) & 0xff;
  myRAM[address + 3] = (value >> 24) & 0xff;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::getDatastreamIncrement(uint8_t index) const
{
  uint16_t address = myDatastreamIncrementBase + index * 4;
  return  myRAM[address + 0]        +
         (myRAM[address + 1] << 8)  +
         (myRAM[address + 2] << 16) +
         (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::getWaveform(uint8_t index) const
{
  uint16_t address = myWaveformBase + index * 4;
  uint32_t result = myRAM[address + 0]        +
                   (myRAM[address + 1] << 8)  +
                   (myRAM[address + 2] << 16) +
                   (myRAM[address + 3] << 24);

  result -= (0x40000000u + 2u * 1024u);

  if(!isCDFJplus())
    if(result >= 4096)
      result &= 4095;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::getSample()
{
  uint16_t address = myWaveformBase;
  return  myRAM[address + 0]        +
         (myRAM[address + 1] << 8)  +
         (myRAM[address + 2] << 16) +
         (myRAM[address + 3] << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::getWaveformSize(uint8_t index) const
{
  return myMusicWaveformSize[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeCDF::readFromDatastream(uint8_t index)
{
  uint32_t pointer = getDatastreamPointer(index);
  uint16_t increment = getDatastreamIncrement(index);
  uint8_t value;

  if(isCDFJplus())
  {
    value = myDisplayImage[pointer >> 16];
    pointer += (increment << 8);
  }
  else
  {
    value = myDisplayImage[pointer >> 20];
    pointer += (increment << 12);
  }

  setDatastreamPointer(index, pointer);
  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t CartridgeCDF::peek(uint16_t address)
{
  address &= 0x0FFF;
  uint8_t peekvalue = myProgramImage[myBankOffset + address];

  if(bankLocked())
    return peekvalue;

  /* JMP FASTJMP: fetch the destination address from the jump stream. */
  if(myFastJumpActive && myJMPoperandAddress == address)
  {
    uint32_t pointer;
    uint8_t value;

    --myFastJumpActive;
    ++myJMPoperandAddress;

    pointer = getDatastreamPointer(myFastJumpStream);
    if(isCDFJplus())
    {
      value = myDisplayImage[pointer >> 16];
      pointer += 0x00010000;
    }
    else
    {
      value = myDisplayImage[pointer >> 20];
      pointer += 0x00100000;
    }
    setDatastreamPointer(myFastJumpStream, pointer);
    return value;
  }

  /* Detect JMP FASTJUMP (JMP $0000 with the stream index in the operand). */
  if(FAST_FETCH_ON(myMode)
     && peekvalue == 0x4C
     && (myProgramImage[myBankOffset + address + 1] & myFastjumpStreamIndexMask) == 0
     && myProgramImage[myBankOffset + address + 2] == 0)
  {
    myFastJumpActive = 2;
    myJMPoperandAddress = address + 1;
    myFastJumpStream = myProgramImage[myBankOffset + address + 1] + JUMPSTREAM_BASE;
    return peekvalue;
  }

  myJMPoperandAddress = 0;

  {
    bool fastfetch;
    if(myFastFetcherOffset)
      fastfetch = (FAST_FETCH_ON(myMode)
                   && myLDAXYimmediateOperandAddress == address
                   && peekvalue >= myRAM[myFastFetcherOffset]
                   && peekvalue <= myRAM[myFastFetcherOffset] + myAmplitudeStream);
    else
      fastfetch = (FAST_FETCH_ON(myMode)
                   && myLDAXYimmediateOperandAddress == address
                   && peekvalue <= myAmplitudeStream);

    if(fastfetch)
    {
      myLDAXYimmediateOperandAddress = LDAXY_OVERRIDE_INACTIVE;
      if(myFastFetcherOffset)
        peekvalue -= myRAM[myFastFetcherOffset];

      if(peekvalue == myAmplitudeStream)
      {
        updateMusicModeDataFetchers();

        if(DIGITAL_AUDIO_ON(myMode))
        {
          uint32_t sampleaddress = getSample()
              + (myMusicCounters[0] >> (isCDFJplus() ? 13 : 21));

          if(sampleaddress < 0x00080000)
            peekvalue = myImage[sampleaddress];
          else if(sampleaddress >= 0x40000000 && sampleaddress < 0x40008000)
            peekvalue = myRAM[sampleaddress - 0x40000000];
          else
            peekvalue = 0;

          if((myMusicCounters[0] & (1 << (isCDFJplus() ? 12 : 20))) == 0)
            peekvalue >>= 4;
          peekvalue &= 0x0f;
        }
        else
        {
          peekvalue = myDisplayImage[getWaveform(0) + (myMusicCounters[0] >> myMusicWaveformSize[0])]
                    + myDisplayImage[getWaveform(1) + (myMusicCounters[1] >> myMusicWaveformSize[1])]
                    + myDisplayImage[getWaveform(2) + (myMusicCounters[2] >> myMusicWaveformSize[2])];
        }
        return peekvalue;
      }
      else
      {
        return readFromDatastream(peekvalue);
      }
    }
  }
  myLDAXYimmediateOperandAddress = LDAXY_OVERRIDE_INACTIVE;

  switch(address)
  {
    case 0x0FF4: bank(isCDFJplus() ? 0 : 6); break;
    case 0x0FF5: bank(isCDFJplus() ? 1 : 0); break;
    case 0x0FF6: bank(isCDFJplus() ? 2 : 1); break;
    case 0x0FF7: bank(isCDFJplus() ? 3 : 2); break;
    case 0x0FF8: bank(isCDFJplus() ? 4 : 3); break;
    case 0x0FF9: bank(isCDFJplus() ? 5 : 4); break;
    case 0x0FFA: bank(isCDFJplus() ? 6 : 5); break;
    case 0x0FFB: bank(isCDFJplus() ? 0 : 6); break;
    default: break;
  }

  if(FAST_FETCH_ON(myMode))
  {
    if((peekvalue == 0xA9) ||
       (myLDXenabled && peekvalue == 0xA2) ||
       (myLDYenabled && peekvalue == 0xA0))
      myLDAXYimmediateOperandAddress = address + 1;
  }

  return peekvalue;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::poke(uint16_t address, uint8_t value)
{
  uint32_t pointer;

  address &= 0x0FFF;
  switch(address)
  {
    case 0x0FF0:  /* DSWRITE */
      pointer = getDatastreamPointer(COMMSTREAM);
      if(isCDFJplus())
      {
        myDisplayImage[pointer >> 16] = value;
        pointer += 0x00010000;
      }
      else
      {
        myDisplayImage[pointer >> 20] = value;
        pointer += 0x00100000;
      }
      setDatastreamPointer(COMMSTREAM, pointer);
      break;

    case 0x0FF1:  /* DSPTR */
      pointer = getDatastreamPointer(COMMSTREAM);
      pointer <<= 8;
      if(isCDFJplus())
      {
        pointer &= 0xff000000;
        pointer |= (value << 16);
      }
      else
      {
        pointer &= 0xf0000000;
        pointer |= (value << 20);
      }
      setDatastreamPointer(COMMSTREAM, pointer);
      break;

    case 0x0FF2:  /* SETMODE */
      myMode = value;
      break;

    case 0x0FF3:  /* CALLFN */
      callFunction(value);
      break;

    case 0x0FF4: bank(isCDFJplus() ? 0 : 6); break;
    case 0x0FF5: bank(isCDFJplus() ? 1 : 0); break;
    case 0x0FF6: bank(isCDFJplus() ? 2 : 1); break;
    case 0x0FF7: bank(isCDFJplus() ? 3 : 2); break;
    case 0x0FF8: bank(isCDFJplus() ? 4 : 3); break;
    case 0x0FF9: bank(isCDFJplus() ? 5 : 4); break;
    case 0x0FFA: bank(isCDFJplus() ? 6 : 5); break;
    case 0x0FFB: bank(isCDFJplus() ? 0 : 6); break;
    default: break;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::scanCDFDriver(uint32_t searchValue)
{
  int i;
  for(i = 0; i < 2048; i += 4)
    if(getUInt32(myImage, i) == searchValue)
      return i;
  return 0xFFFFFFFFu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCDF::setupVersion()
{
  uint32_t cdfjOffset;

  if((cdfjOffset = scanCDFDriver(0x53554c50)) != 0xFFFFFFFFu &&
     getUInt32(myImage, cdfjOffset + 4) == 0x4a464443 &&
     getUInt32(myImage, cdfjOffset + 8) == 0x00000001)
  {
    int i;
    myCDFSubtype = CDFSubtype_CDFJplus;
    myAmplitudeStream = 0x23;
    myFastjumpStreamIndexMask = 0xfe;
    myDatastreamBase = 0x0098;
    myDatastreamIncrementBase = 0x0124;
    myFastFetcherOffset = 0;
    myWaveformBase = 0x01b0;

    for(i = 0; i < 2048; i += 4)
    {
      uint32_t cdfjValue = getUInt32(myImage, i);
      if(cdfjValue == 0x135200A2)
        myLDXenabled = true;
      if(cdfjValue == 0x135200A0)
        myLDYenabled = true;
      if((cdfjValue & 0xFFFFFF00) == 0xE2422000)
        myFastFetcherOffset = i;
    }
    return;
  }

  {
    uint8_t subversion = 0;
    int i;
    for(i = 0; i < 2048; i += 4)
    {
      if(myImage[i + 0] == 0x43 && myImage[i + 4] == 0x43 && myImage[i + 8] == 0x43)
        if(myImage[i + 1] == 0x44 && myImage[i + 5] == 0x44 && myImage[i + 9] == 0x44)
          if(myImage[i + 2] == 0x46 && myImage[i + 6] == 0x46 && myImage[i + 10] == 0x46)
          {
            subversion = myImage[i + 3];
            break;
          }
    }

    switch(subversion)
    {
      case 0x4a:
        myCDFSubtype = CDFSubtype_CDFJ;
        myAmplitudeStream = 0x23;
        myFastjumpStreamIndexMask = 0xfe;
        myDatastreamBase = 0x0098;
        myDatastreamIncrementBase = 0x0124;
        myWaveformBase = 0x01b0;
        break;
      case 0:
        myCDFSubtype = CDFSubtype_CDF0;
        myAmplitudeStream = 0x22;
        myFastjumpStreamIndexMask = 0xff;
        myDatastreamBase = 0x06e0;
        myDatastreamIncrementBase = 0x0768;
        myWaveformBase = 0x07f0;
        break;
      default:
        myCDFSubtype = CDFSubtype_CDF1;
        myAmplitudeStream = 0x22;
        myFastjumpStreamIndexMask = 0xff;
        myDatastreamBase = 0x00a0;
        myDatastreamIncrementBase = 0x0128;
        myWaveformBase = 0x01b0;
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::isCDFJplus() const
{
  return myCDFSubtype == CDFSubtype_CDFJplus;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::ramSize() const
{
  return isCDFJplus() ? 32u * 1024u : 8u * 1024u;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t CartridgeCDF::romSize() const
{
  return isCDFJplus() ? mySize : 32u * 1024u;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::save(Serializer& out) const
{
  try
  {
    out.putShort(myBankOffset);
    out.putByte(myMode);
    out.putByte(myFastJumpActive);
    out.putShort(myLDAXYimmediateOperandAddress);
    out.putShort(myJMPoperandAddress);
    out.putByteArray(myRAM, sizeof(myRAM));
    out.putIntArray(myMusicCounters, 3);
    out.putIntArray(myMusicFrequencies, 3);
    out.putByteArray(myMusicWaveformSize, 3);
    out.putInt(myAudioCycles);
    out.putInt(myFractionalClocksFrac);
    out.putInt(myARMCycles);
    CartridgeARM::saveArmState(out);
  }
  catch(...)
  {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeCDF::load(Serializer& in)
{
  try
  {
    myBankOffset = in.getShort();
    myMode = in.getByte();
    myFastJumpActive = in.getByte();
    myLDAXYimmediateOperandAddress = in.getShort();
    myJMPoperandAddress = in.getShort();
    in.getByteArray(myRAM, sizeof(myRAM));
    in.getIntArray(myMusicCounters, 3);
    in.getIntArray(myMusicFrequencies, 3);
    in.getByteArray(myMusicWaveformSize, 3);
    myAudioCycles = in.getInt();
    myFractionalClocksFrac = in.getInt();
    myARMCycles = in.getInt();
    CartridgeARM::loadArmState(in);
  }
  catch(...)
  {
    return false;
  }

  bank(myCurrentBank);
  return true;
}
