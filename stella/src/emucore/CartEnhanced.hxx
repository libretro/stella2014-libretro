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

#ifndef CARTRIDGE_ENHANCED_HXX
#define CARTRIDGE_ENHANCED_HXX

#include "bspf.hxx"
#include "Cart.hxx"
#include "System.hxx"

/**
  A generalized bankswitching base class, backported from the modern Stella
  "CartridgeEnhanced" for use in this (C++98) tree. It provides a common
  ROM (and optionally banked RAM) segment-mapping engine so that individual
  mappers only need to describe their hotspots via checkSwitchBank().

  The 4K address space is divided into 2^(12 - myBankShift) segments, each
  of size (1 << myBankShift). Each segment independently points at a ROM
  bank; if the cart also has banked RAM, banks at or above romBankCount()
  address the RAM banks, which are half the size of a ROM bank and expose a
  read port and a write port.

  Subclasses set the geometry (myBankShift, myRamSize, myRamBankCount, ...)
  in their constructor and implement checkSwitchBank() to decode their
  hotspots. This mirrors the modern class closely, minus PlusROM and the
  debugger access counters, which this tree does not have.

  @author  Thomas Jentzsch, Stephen Anthony (original); backported for the
           2014 core
*/
class CartridgeEnhanced : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image.

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
      @param bsSize    The size expected by the bankswitching scheme
    */
    CartridgeEnhanced(const uint8_t* image, uint32_t size,
                      const Settings& settings, uint32_t bsSize);

    virtual ~CartridgeEnhanced();

  public:
    /**
      Reset device to its power-on state.
    */
    virtual void reset();

    /**
      Install cartridge in the specified system.
    */
    virtual void install(System& system);

    /**
      Install pages for the specified bank in the first segment.
    */
    virtual bool bank(uint16_t bank);

    /**
      Install pages for the specified bank in the specified segment.
    */
    virtual bool bank(uint16_t bank, uint16_t segment);

    /**
      Get the current bank for the first (or given) segment.
    */
    virtual uint16_t bank() const;

    /**
      Query the number of ROM banks supported by the cartridge.
    */
    virtual uint16_t bankCount() const;

    /**
      Total number of ROM banks (== bankCount()).
    */
    uint16_t romBankCount() const;

    /**
      Number of banked RAM banks.
    */
    uint16_t ramBankCount() const;

    /**
      Patch the cartridge ROM/RAM.
    */
    virtual bool patch(uint16_t address, uint8_t value);

    /**
      Access the internal ROM image for this cartridge.
    */
    virtual const uint8_t* getImage(int& size) const;

    /**
      Save the current state of this cart to the given Serializer.
    */
    virtual bool save(Serializer& out) const;

    /**
      Load the current state of this cart from the given Serializer.
    */
    virtual bool load(Serializer& in);

    /**
      Get the byte at the specified address.
    */
    virtual uint8_t peek(uint16_t address);

    /**
      Change the byte at the specified address to the given value.
    */
    virtual bool poke(uint16_t address, uint8_t value);

  protected:
    // The size of a ROM bank expressed as a shift (12 => one 4K segment).
    uint16_t myBankShift;

    // The size of a ROM bank (1 << myBankShift) and its address mask.
    uint16_t myBankSize;
    uint16_t myBankMask;

    // The size of a RAM bank expressed as a shift (usually myBankShift - 1).
    uint16_t myRamBankShift;

    // The total size of the (extra or banked) RAM in bytes.
    uint32_t myRamSize;

    // The number of banked RAM banks (0 for plain extra RAM).
    uint16_t myRamBankCount;

    // RAM address mask.
    uint16_t myRamMask;

    // The number of ROM bank segments in the 4K space.
    uint16_t myBankSegs;

    // The ROM offset applied when extra (non-banked) RAM sits inside a bank.
    uint16_t myRomOffset;

    // The read and write port offsets for extra RAM within a segment.
    uint16_t myWriteOffset;
    uint16_t myReadOffset;

    // Whether the write port is at the high half of the RAM window.
    bool myRamWpHigh;

    // Whether ROM reads use direct page access (true) or route through
    // peek() (false, for schemes with a per-read side effect).
    bool myDirectPeek;

    // Pointer to a copy of the entire ROM image (followed by RAM banks).
    uint8_t* myImage;

    // Size of the ROM image (bsSize).
    uint32_t mySize;

    // The current byte offset for each ROM/RAM segment.
    uint32_t* myCurrentSegOffset;

    // The RAM area.
    uint8_t* myRAM;

  protected:
    static const uint16_t ADDR_MASK  = 0x1FFF;
    static const uint16_t ROM_OFFSET = 0x1000;
    static const uint16_t ROM_MASK   = 0x0FFF;

  protected:
    /**
      Decode the mapper's hotspots for the given address; switch banks as
      needed. Returns true if the access hit a hotspot.
    */
    virtual bool checkSwitchBank(uint16_t address, uint8_t value) = 0;

    /**
      Number of ROM bank segments for the current geometry.
    */
    virtual uint16_t calcNumSegments() const;

    /**
      The power-on bank (default 0).
    */
    virtual uint16_t getStartBank() const { return 0; }

    /**
      True if the address currently maps into a RAM bank.
    */
    bool isRamBank(uint16_t address) const;

    // ROM segment offset for a ROM-window address
    uint32_t romAddressSegmentOffset(uint16_t address) const
    {
      return myCurrentSegOffset[((address & ROM_MASK) >> myBankShift) % myBankSegs];
    }

    // RAM segment offset for a ROM-window address
    uint16_t ramAddressSegmentOffset(uint16_t address) const
    {
      return (uint16_t)((myCurrentSegOffset[((address & ROM_MASK) >> myBankShift) % myBankSegs]
             - mySize) >> (myBankShift - myRamBankShift));
    }

  private:
    // Following constructors and assignment operators not supported
    CartridgeEnhanced();
    CartridgeEnhanced(const CartridgeEnhanced&);
    CartridgeEnhanced& operator=(const CartridgeEnhanced&);
};

#endif
