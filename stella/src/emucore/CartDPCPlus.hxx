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
// $Id: CartDPCPlus.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef CARTRIDGE_DPC_PLUS_HXX
#define CARTRIDGE_DPC_PLUS_HXX

class System;
#ifdef THUMB_SUPPORT
struct Thumbulator;
#endif

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for DPC+, derived from Pitfall II.  There are six 4K
  program banks, a 4K display bank, 1K frequency table and the DPC chip.
  DPC chip access is mapped to $1000 - $1080 ($1000 - $103F is read port,
  $1040 - $107F is write port).

  For complete details on the DPC chip see David P. Crane's United States
  Patent Number 4,644,495.

  @author  Darrell Spice Jr, Fred Quimby, Stephen Anthony, Bradford W. Mott
  @version $Id: CartDPCPlus.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class CartridgeDPCPlus : public Cartridge
{
  friend class CartridgeDPCPlusWidget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeDPCPlus(const uint8_t* image, uint32_t size, const Settings& settings);
 
    /**
      Destructor
    */
    virtual ~CartridgeDPCPlus();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset();

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system);

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uint16_t bank);

    /**
      Get the current bank.
    */
    uint16_t bank() const;

    /**
      Query the number of banks supported by the cartridge.
    */
    uint16_t bankCount() const;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uint16_t address, uint8_t value);

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uint8_t* getImage(int& size) const;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const { return "CartridgeDPC+"; }

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uint8_t peek(uint16_t address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uint16_t address, uint8_t value);

  private:
    /** 
      Sets the initial state of the DPC pointers and RAM
    */
    void setInitialState();

    /** 
      Clocks the random number generator to move it to its next state
    */
    void clockRandomNumberGenerator();
  
    /** 
      Clocks the random number generator to move it to its prior state
    */
    void priorClockRandomNumberGenerator();

    /** 
      Updates any data fetchers in music mode based on the number of
      CPU cycles which have passed since the last update.
    */
    void updateMusicModeDataFetchers();

    /** 
      Call Special Functions
    */
    void callFunction(uint8_t value);

  private:
    // The ROM image and size
    uint8_t* myImage;
    uint32_t mySize;

    // Pointer to the 24K program ROM image of the cartridge
    uint8_t* myProgramImage;

    // Pointer to the 4K display ROM image of the cartridge
    uint8_t* myDisplayImage;

    // The DPC 8k RAM image
    uint8_t myDPCRAM[8192];

#ifdef THUMB_SUPPORT
    // Pointer to the Thumb ARM emulator object
    Thumbulator* myThumbEmulator;
#endif

    // Pointer to the 1K frequency table
    uint8_t* myFrequencyImage;

    // Indicates which bank is currently active
    uint16_t myCurrentBank;
  
    // The top registers for the data fetchers
    uint8_t myTops[8];

    // The bottom registers for the data fetchers
    uint8_t myBottoms[8];

    // The counter registers for the data fetchers
    uint16_t myCounters[8];
  
    // The counter registers for the fractional data fetchers
    uint32_t myFractionalCounters[8];

    // The fractional increments for the data fetchers
    uint8_t myFractionalIncrements[8];

    // The Fast Fetcher Enabled flag
    bool myFastFetch;
  
    // Flags that last byte peeked was A9 (LDA #)
    bool myLDAimmediate;

    // Parameter for special functions
    uint8_t myParameter[8];

    // Parameter pointer for special functions
    uint8_t myParameterPointer;

    // The music mode counters
    uint32_t myMusicCounters[3];

    // The music frequency
    uint32_t myMusicFrequencies[3];
  
    // The music waveforms
    uint16_t myMusicWaveforms[3];
  
    // The random number generator register
    uint32_t myRandomNumber;

    // System cycle count when the last update to music data fetchers occurred
    int32_t mySystemCycles;

    // Fractional DPC music OSC clocks unused during the last update,
    // as an integer remainder in units of 1/myDpcClockDen of an OSC clock
    uint32_t myFractionalClocks;

    // DPC music-oscillator clock rate as an exact integer ratio
    // (OSC clocks per CPU cycle = num/den), chosen from the TV format
    uint32_t myDpcClockNum;
    uint32_t myDpcClockDen;

  public:
    void setDpcClockRate(uint32_t num, uint32_t den);

  private:
};

#endif
