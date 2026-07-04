//============================================================================
// CartCDF for stella2014 - backported from Stella 7, adapted to the 2014
// Cartridge API (uint16_t/uint8_t, no debugger widgets, no PlusROM) and to
// the C Thumbulator. Integer music clock (no double) for determinism.
//============================================================================

#ifndef CARTRIDGE_CDF_HXX
#define CARTRIDGE_CDF_HXX

class System;

#include "bspf.hxx"
#include "CartARM.hxx"

class CartridgeCDF : public CartridgeARM
{
  public:
    enum CDFSubtype {
      CDFSubtype_CDF0,
      CDFSubtype_CDF1,
      CDFSubtype_CDFJ,
      CDFSubtype_CDFJplus
    };

  public:
    CartridgeCDF(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeCDF();

  public:
    void reset();
    void install(System& system);
    bool bank(uint16_t bank);
    uint16_t bank() const;
    uint16_t bankCount() const;
    bool patch(uint16_t address, uint8_t value);
    const uint8_t* getImage(int& size) const;
    bool save(Serializer& out) const;
    bool load(Serializer& in);
    string name() const { return "CartridgeCDF"; }
    uint32_t thumbCallback(uint8_t function, uint32_t value1, uint32_t value2);

  public:
    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

  private:
    void setInitialState();
    void updateMusicModeDataFetchers();
    void callFunction(uint8_t value);
    uint32_t getDatastreamPointer(uint8_t index) const;
    void setDatastreamPointer(uint8_t index, uint32_t value);
    uint32_t getDatastreamIncrement(uint8_t index) const;
    uint32_t getWaveform(uint8_t index) const;
    uint32_t getSample();
    uint32_t getWaveformSize(uint8_t index) const;
    uint8_t readFromDatastream(uint8_t index);
    uint32_t scanCDFDriver(uint32_t searchValue);
    void setupVersion();
    bool isCDFJplus() const;
    uint32_t ramSize() const;
    uint32_t romSize() const;

  private:
    uint8_t* myImage;
    uint32_t mySize;
    uint8_t* myProgramImage;
    uint8_t* myDisplayImage;
    uint8_t* myDriverImage;
    uint8_t myRAM[32 * 1024];
    uint16_t myBankOffset;
    uint16_t myCurrentBank;
    uint32_t myAudioCycles;
    uint32_t myARMCycles;
    uint32_t myMusicCounters[3];
    uint32_t myMusicFrequencies[3];
    uint8_t  myMusicWaveformSize[3];
    uint32_t myFractionalClocksFrac;  // integer fractional-clock remainder
    uint8_t  myMode;
    uint16_t myLDAXYimmediateOperandAddress;
    bool     myLDXenabled;
    bool     myLDYenabled;
    uint16_t myFastFetcherOffset;
    uint16_t myJMPoperandAddress;
    uint8_t  myFastJumpActive;
    uint16_t myDatastreamBase;
    uint16_t myDatastreamIncrementBase;
    uint16_t myWaveformBase;
    uint8_t  myAmplitudeStream;
    uint8_t  myFastjumpStreamIndexMask;
    uint8_t  myFastJumpStream;
    uint32_t myClockRate;
    CDFSubtype myCDFSubtype;

  private:
    CartridgeCDF();
    CartridgeCDF(const CartridgeCDF&);
    CartridgeCDF& operator = (const CartridgeCDF&);
};

#endif
