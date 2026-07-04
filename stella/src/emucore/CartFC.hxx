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

#ifndef CARTRIDGE_FC_HXX
#define CARTRIDGE_FC_HXX

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for Amiga's Power Play Arcade Video Game Album. In
  the full (32K) form there are eight 4K banks. The target bank is assembled
  from two register writes and then committed by a separate access:

    write to $1FF8 : sets the two lowest bits of the target bank
    write to $1FF9 : sets the high bits of the target bank
    access $1FFC   : commits the switch to the assembled target bank

  Smaller images (down to 4K) use the same registers with fewer banks.

  @author  Thomas Jentzsch (original); backported for the 2014 core
*/
class CartridgeFC : public Cartridge
{
  friend class CartridgeFCWidget;

  public:
    /**
      Create a new cartridge using the specified image.

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeFC(const uint8_t* image, uint32_t size, const Settings& settings);

    /**
      Destructor
    */
    virtual ~CartridgeFC();

  public:
    /**
      Reset device to its power-on state.
    */
    void reset();

    /**
      Install cartridge in the specified system.

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
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of this cart from the given Serializer.
    */
    bool load(Serializer& in);

    /**
      Get a descriptor for the device name (used in error checking).
    */
    string name() const { return "CartridgeFC"; }

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uint8_t peek(uint16_t address);

    /**
      Change the byte at the specified address to the given value.

      @return  True if the poke changed the device address space, else false
    */
    bool poke(uint16_t address, uint8_t value);

  private:
    // Handle the target-bank registers and the commit hotspot.
    void checkSwitchBank(uint16_t address, uint8_t value);

  private:
    // Pointer to a copy of the entire ROM image
    uint8_t* myImage;

    // Size of the ROM image
    uint32_t mySize;

    // Number of 4K banks in the image
    uint16_t myNumBanks;

    // Indicates which bank is currently active
    uint16_t myCurrentBank;

    // The bank that will be selected when the commit hotspot is accessed
    uint16_t myTargetBank;

  private:
    // Following constructors and assignment operators not supported
    CartridgeFC();
    CartridgeFC(const CartridgeFC&);
    CartridgeFC& operator=(const CartridgeFC&);
};

#endif
