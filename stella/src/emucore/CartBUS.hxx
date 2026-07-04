//============================================================================
// CartBUS for stella2014 - backported from Stella 7, adapted to the 2014
// Cartridge API and the C Thumbulator. Integer music clock (no double).
//============================================================================

#ifndef CARTRIDGE_BUS_HXX
#define CARTRIDGE_BUS_HXX

class System;

#include "bspf.hxx"
#include "CartARM.hxx"

class CartridgeBUS : public CartridgeARM
{
  public:
    enum BUSSubtype {
      BUSSubtype_BUS0,  // very old demos, not supported
      BUSSubtype_BUS1,  // draconian
      BUSSubtype_BUS2,  // 128bus, chronocolour, parrot
      BUSSubtype_BUS3   // rpg
    };

  public:
    CartridgeBUS(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeBUS();

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
    string name() const { return "CartridgeBUS"; }
    uint32_t thumbCallback(uint8_t function, uint32_t value1, uint32_t value2);

  public:
    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

  private:
    void setInitialState();
    void updateMusicModeDataFetchers();
    void callFunction(uint8_t value);
    uint8_t busOverdrive(uint16_t address);
    uint32_t getDatastreamPointer(uint8_t index) const;
    void setDatastreamPointer(uint8_t index, uint32_t value);
    uint32_t getDatastreamIncrement(uint8_t index) const;
    void setDatastreamIncrement(uint8_t index, uint32_t value);
    uint32_t getAddressMap(uint8_t index) const;
    void setAddressMap(uint8_t index, uint32_t value);
    uint32_t getWaveform(uint8_t index) const;
    uint32_t getSample();
    uint32_t getWaveformSize(uint8_t index) const;
    uint8_t readFromDatastream(uint8_t index);
    uint32_t scanBUSDriver(uint32_t searchValue);
    void setupVersion();

  private:
    uint8_t* myImage;
    uint32_t mySize;
    uint8_t* myProgramImage;
    uint8_t* myDisplayImage;
    uint8_t* myDriverImage;
    uint8_t myRAM[8 * 1024];
    uint16_t myBankOffset;
    uint16_t myCurrentBank;
    uint16_t myBusOverdriveAddress;
    uint16_t mySTYZeroPageAddress;
    uint16_t myJMPoperandAddress;
    uint32_t myAudioCycles;
    uint32_t myARMCycles;
    uint16_t myDatastreamBase;
    uint16_t myDatastreamIncrementBase;
    uint16_t myDatastreamMapBase;
    uint16_t myWaveformBase;
    uint32_t myMusicCounters[3];
    uint32_t myMusicFrequencies[3];
    uint8_t  myMusicWaveformSize[3];
    uint32_t myFractionalClocksFrac;  // integer fractional-clock remainder
    uint32_t myClockRate;             // music-clock denominator (per format)
    uint8_t  myMode;
    uint8_t  myFastJumpActive;
    BUSSubtype myBUSSubtype;

  private:
    CartridgeBUS();
    CartridgeBUS(const CartridgeBUS&);
    CartridgeBUS& operator = (const CartridgeBUS&);
};

#endif
