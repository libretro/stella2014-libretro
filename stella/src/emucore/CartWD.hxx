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

#ifndef CARTRIDGE_WD_HXX
#define CARTRIDGE_WD_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#include "System.hxx"

/**
  Cartridge class for the "Wickstead Design" prototype cart (Pink Panther).
  The ROM is 8K with 64 bytes of RAM. The 4K cartridge address space is
  broken into four 1K segments; accessing a hotspot at $0030-$003F in TIA
  address space selects one of eight fixed arrangements of 1K banks that
  are mapped into all four segments at once:

    $0030, $0038: 0,0,1,3      $0034, $003C: 0,0,6,7
    $0031, $0039: 0,1,2,3      $0035, $003D: 0,1,7,6
    $0032, $003A: 4,5,6,7      $0036, $003E: 2,3,4,5
    $0033, $003B: 7,4,2,3      $0037, $003F: 6,0,5,1

  A hotspot read only takes effect a few (>3) cycles later, so the switch
  is initiated on the triggering read and committed on a subsequent access.

  The 64 bytes of RAM are at $1000-$103F (read) and $1040-$107F (write).

  @author  Stephen Anthony, Thomas Jentzsch (original); backported for the
           2014 core
*/
class CartridgeWD : public Cartridge
{
  friend class CartridgeWDWidget;

  public:
    /**
      Create a new cartridge using the specified image.

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeWD(const uint8_t* image, uint32_t size, const Settings& settings);

    /**
      Destructor
    */
    virtual ~CartridgeWD();

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
      Install the given bank arrangement (0..7) in the system.

      @param bank The bank arrangement that should be installed
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
    string name() const { return "CartridgeWD"; }

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
    // Map one 1K bank into one 1K segment (0..3).
    void segment(uint16_t bank, uint16_t seg);

  private:
    // Pointer to a copy of the entire ROM image
    uint8_t* myImage;

    // Size of the ROM image
    uint32_t mySize;

    // The 64 bytes of RAM
    uint8_t myRAM[64];

    // Indicates which bank arrangement is currently active
    uint16_t myCurrentBank;

    // The 1K ROM bank currently mapped into each of the four 1K segments
    uint16_t mySegmentBank[4];

    // The cycle at which a pending bankswitch was initiated, and the bank
    // it will switch to (0xF0 means "no switch pending")
    uint32_t myCyclesAtBankswitchInit;
    uint16_t myPendingBank;

    // Saved page access for the hotspot region (TIA), so accesses there
    // can be forwarded to the underlying device
    System::PageAccess myHotSpotPageAccess;

    // The bank arrangement table: each entry lists the 1K bank mapped into
    // segments 0,1,2,3 respectively.
    struct BankOrg { uint8_t zero, one, two, three; };
    static const BankOrg ourBankOrg[8];

  private:
    // Following constructors and assignment operators not supported
    CartridgeWD();
    CartridgeWD(const CartridgeWD&);
    CartridgeWD& operator=(const CartridgeWD&);
};

#endif
