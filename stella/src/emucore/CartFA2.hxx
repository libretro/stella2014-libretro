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
// $Id: CartFA2.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef CARTRIDGEFA2_HXX
#define CARTRIDGEFA2_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  This is an extended version of the CBS RAM Plus bankswitching scheme
  supported by the Harmony cartridge.
  
  There are six (or seven) 4K banks and 256 bytes of RAM.  The 256 bytes
  of RAM can be loaded/saved to Harmony cart flash, which is emulated by
  storing in a file.

  For 29K versions of the scheme, the first 1K is ARM code
  (implements actual bankswitching on the Harmony cart), which is
  completely ignored by the emulator.

  @author  Chris D. Walton
  @version $Id: CartFA2.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class CartridgeFA2 : public Cartridge
{
  friend class CartridgeFA2Widget;

  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param osystem   A reference to the OSystem currently in use
    */
    CartridgeFA2(const uint8_t* image, uint32_t size, const OSystem& osystem);
 
    /**
      Destructor
    */
    virtual ~CartridgeFA2();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

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
    string name() const { return "CartridgeFA2"; }

    /**
      Informs the cartridge about the name of the ROM file used when
      creating this cart.

      @param name  The properties file name of the ROM
    */
    void setRomName(const string& name);

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
      Either load or save internal RAM to Harmony flash (represented by
      a file in emulation).

      @return  The value at $FF4 with bit 6 set or cleared (depending on
               whether the RAM access was busy or successful)
    */
    uint8_t ramReadWrite();

    /**
      Modify Harmony flash directly (represented by a file in emulation),
      ignoring any timing emulation.  This is for use strictly in the
      debugger, so you can have low-level access to the Flash media.

      @param operation  0 for erase, 1 for read, 2 for write
    */
    void flash(uint8_t operation);

  private:
    // OSsytem currently in use
    const OSystem& myOSystem;

    // Indicates which bank is currently active
    uint16_t myCurrentBank;

    // The 24K/28K ROM image of the cartridge
    uint8_t* myImage;

    // The 256 bytes of RAM on the cartridge
    uint8_t myRAM[256];

    // The time after which the first request of a load/save operation
    // will actually be completed
    // Due to flash RAM constraints, a read/write isn't instantaneous,
    // so we need to emulate the delay as well
    uint64_t myRamAccessTimeout;

    // Full pathname of the file to use when emulating load/save
    // of internal RAM to Harmony cart flash
    string myFlashFile;

    // Size of the ROM image
    uint32_t mySize;
};

#endif
