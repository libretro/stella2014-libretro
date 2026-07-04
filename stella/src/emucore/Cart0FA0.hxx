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

#ifndef CARTRIDGE_0FA0_HXX
#define CARTRIDGE_0FA0_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#include "System.hxx"

/**
  Cartridge class used for some Brazilian (Digivision) 8K bankswitched
  games. Bankswitching is triggered by accessing addresses $06A0 (select
  the lower 4K bank) and $06C0 (select the upper 4K bank). Because these
  hotspots overlap the TIA/RIOT mirror region, accesses to them must be
  forwarded to the underlying device. Address lines A11 and A8 are not
  decoded, so the hotspots also appear at the corresponding mirrors.

  @author  Thomas Jentzsch (original); backported for the 2014 core
*/
class Cartridge0FA0 : public Cartridge
{
  friend class Cartridge0FA0Widget;

  public:
    /**
      Create a new cartridge using the specified image.

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    Cartridge0FA0(const uint8_t* image, uint32_t size, const Settings& settings);

    /**
      Destructor
    */
    virtual ~Cartridge0FA0();

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
    string name() const { return "Cartridge0FA0"; }

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
    // Switch bank if the (masked) address hits a hotspot.
    void checkSwitchBank(uint16_t address);

  private:
    // Pointer to a copy of the entire ROM image
    uint8_t* myImage;

    // Size of the ROM image
    uint32_t mySize;

    // Indicates which bank is currently active
    uint16_t myCurrentBank;

    // Saved page access for the hotspot region (TIA/RIOT), so accesses
    // there can be forwarded to the underlying device
    System::PageAccess myHotSpotPageAccess;

  private:
    // Following constructors and assignment operators not supported
    Cartridge0FA0();
    Cartridge0FA0(const Cartridge0FA0&);
    Cartridge0FA0& operator=(const Cartridge0FA0&);
};

#endif
