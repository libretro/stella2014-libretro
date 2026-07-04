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

#ifndef CARTRIDGE_03E0_HXX
#define CARTRIDGE_03E0_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#include "System.hxx"

/**
  Cartridge class for Parker Brothers' 8K games with the special Brazilian
  "03E0" bankswitching. The 4K cartridge address space is split into four
  1K segments. The desired 1K bank of the ROM is selected by accessing a
  hotspot in $0380..$03FF (which overlaps the TIA/RIOT region):

    A4 == 0 ($03E0) loads the bank number for segment #0
    A5 == 0 ($03D0) loads the bank number for segment #1
    A6 == 0 ($03B0) loads the bank number for segment #2

  with A0..A2 giving the bank number (0..7). The last 1K segment always
  points to the last 1K of the ROM image.

  @author  Thomas Jentzsch (original); backported for the 2014 core
*/
class Cartridge03E0 : public Cartridge
{
  friend class Cartridge03E0Widget;

  public:
    /**
      Create a new cartridge using the specified image.

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    Cartridge03E0(const uint8_t* image, uint32_t size, const Settings& settings);

    /**
      Destructor
    */
    virtual ~Cartridge03E0();

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
      Install pages for the specified bank in the system. Not used directly
      by this scheme (segments are switched individually), but required by
      the Cartridge interface.
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
    string name() const { return "Cartridge03E0"; }

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
    // Handle a hotspot access; switch any segments it selects.
    void checkSwitchBank(uint16_t address);

    // Map the given 1K bank into the given 1K segment (0..2).
    void segment(uint16_t bank, uint16_t seg);

  private:
    // Pointer to a copy of the entire ROM image
    uint8_t* myImage;

    // Size of the ROM image
    uint32_t mySize;

    // The currently selected 1K slice for each of the four segments
    uint16_t myCurrentSlice[4];

    // Number of 1K banks in the image
    uint16_t myNumBanks;

    // Saved page access for the hotspot region (TIA/RIOT)
    System::PageAccess myHotSpotPageAccess[2];

  private:
    // Following constructors and assignment operators not supported
    Cartridge03E0();
    Cartridge03E0(const Cartridge03E0&);
    Cartridge03E0& operator=(const Cartridge03E0&);
};

#endif
