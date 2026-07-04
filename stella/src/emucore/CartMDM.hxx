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

#ifndef CARTRIDGE_MDM_HXX
#define CARTRIDGE_MDM_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#include "System.hxx"

/**
  Cartridge class for the "Menu Driven Megacart" (Edwin Blink). In this
  (modified) scheme the hotspots are read/write at $0800-$0BFF, where the
  low byte selects the 4K bank to switch to. Up to 128 banks (512K) are
  supported; selecting bank 128 or above locks all further bankswitching
  until a reset. Because the hotspots overlap the TIA/RIOT region, accesses
  there are forwarded to the underlying device.

  @author  Stephen Anthony, Thomas Jentzsch (original), based on the 0840
           scheme by Fred X. Quimby; backported for the 2014 core
*/
class CartridgeMDM : public Cartridge
{
  friend class CartridgeMDMWidget;

  public:
    /**
      Create a new cartridge using the specified image.

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeMDM(const uint8_t* image, uint32_t size, const Settings& settings);

    /**
      Destructor
    */
    virtual ~CartridgeMDM();

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
    string name() const { return "CartridgeMDM"; }

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
    // Switch bank if the address hits a hotspot.
    void checkSwitchBank(uint16_t address);

  private:
    // Pointer to a copy of the entire ROM image
    uint8_t* myImage;

    // Size of the ROM image
    uint32_t mySize;

    // Number of 4K banks in the image
    uint16_t myNumBanks;

    // Indicates which bank is currently active
    uint16_t myCurrentBank;

    // Whether banking has been disabled (by selecting bank 128+)
    bool myBankingDisabled;

    // Saved page access for the hotspot region (TIA/RIOT), so accesses
    // there can be forwarded to the underlying device
    System::PageAccess myHotSpotPageAccess[8];

  private:
    // Following constructors and assignment operators not supported
    CartridgeMDM();
    CartridgeMDM(const CartridgeMDM&);
    CartridgeMDM& operator=(const CartridgeMDM&);
};

#endif
