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
// $Id: CartAR.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef CARTRIDGEAR_HXX
#define CARTRIDGEAR_HXX

class M6502;
class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  This is the cartridge class for Arcadia (aka Starpath) Supercharger 
  games.  Christopher Salomon provided most of the technical details 
  used in creating this class.  A good description of the Supercharger
  is provided in the Cuttle Cart's manual.

  The Supercharger has four 2K banks.  There are three banks of RAM 
  and one bank of ROM.  All 6K of the RAM can be read and written.

  @author  Bradford W. Mott
  @version $Id: CartAR.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class CartridgeAR : public Cartridge
{
  friend class CartridgeARWidget;

  public:
    /**
      Create a new cartridge using the specified image and size

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeAR(const uint8_t* image, uint32_t size, const Settings& settings);

    /**
      Destructor
    */
    virtual ~CartridgeAR();

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
    string name() const { return "CartridgeAR"; }

  public:
    /**
      Get the byte at the specified address

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
    // Handle a change to the bank configuration
    bool bankConfiguration(uint8_t configuration);

    // Compute the sum of the array of bytes
    uint8_t checksum(uint8_t* s, uint16_t length);

    // Load the specified load into SC RAM
    void loadIntoRAM(uint8_t load);

    // Sets up a "dummy" BIOS ROM in the ROM bank of the cartridge
    void initializeROM();

  private:
    // Pointer to the 6502 processor in the system
    M6502* my6502;

    // Indicates the offset within the image for the corresponding bank
    uint32_t myImageOffset[2];

    // The 6K of RAM and 2K of ROM contained in the Supercharger
    uint8_t myImage[8192];

    // The 256 byte header for the current 8448 byte load
    uint8_t myHeader[256];

    // Size of the ROM image
    uint32_t mySize;

    // All of the 8448 byte loads associated with the game 
    uint8_t* myLoadImages;

    // Indicates how many 8448 loads there are
    uint8_t myNumberOfLoadImages;

    // Indicates if the RAM is write enabled
    bool myWriteEnabled;

    // Indicates if the ROM's power is on or off
    bool myPower;

    // Indicates when the power was last turned on
    int32_t myPowerRomCycle;

    // Data hold register used for writing
    uint8_t myDataHoldRegister;

    // Indicates number of distinct accesses when data hold register was set
    uint32_t myNumberOfDistinctAccesses;

    // Indicates if a write is pending or not
    bool myWritePending;

    uint16_t myCurrentBank;

    // Fake SC-BIOS code to simulate the Supercharger load bars
    static uint8_t ourDummyROMCode[294];

    // Default 256-byte header to use if one isn't included in the ROM
    // This data comes from z26
    static const uint8_t ourDefaultHeader[256];
};

#endif
