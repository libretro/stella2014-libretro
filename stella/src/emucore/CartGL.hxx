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

#ifndef CARTRIDGE_GL_HXX
#define CARTRIDGE_GL_HXX

#include "bspf.hxx"
#include "CartEnhanced.hxx"

/**
  Cartridge class used for Control Video Corporation's GameLine Master Module.

  The 4K address space is divided into four 1K slices. Each slice can select
  one of four 1K ROM banks or one of twelve 1K RAM banks, and (for RAM) a
  read/write mode:

  - $0480.. selects the bank mapped into slice 0
  - $0580.. selects the bank mapped into slice 1
  - $0880.. selects the bank mapped into slice 2
  - $0980.. selects the bank mapped into slice 3
      - bits 0..3: mapped 1K bank (0..3 = ROM bank, 4..15 = RAM bank)
      - bit 5:     0 = read, 1 = write (RAM only)
  - $0c80.. / $0d80.. control the modem; only the PROM enable is emulated.

  The scheme supports 4K ROM and 2K RAM (a 6K image additionally carries the
  initial RAM contents in the top 2K).

  Reimplemented on CartridgeEnhanced for the 2014 core, using the data-bus
  open-bus model rather than the modern random-value model, and without the
  modem, which the standalone core also leaves unimplemented.

  @author  Thomas Jentzsch (original); 2014 port
*/
class CartridgeGL : public CartridgeEnhanced
{
  friend class CartridgeGLWidget;

  public:
    CartridgeGL(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeGL();

  public:
    void reset();
    void install(System& system);

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

    bool save(Serializer& out) const;
    bool load(Serializer& in);

    string name() const { return "CartridgeGL"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x0480; }

    uint16_t calcNumSegments() const { return 4; }

  private:
    // 1K ROM/RAM segments (four slices)
    static const uint16_t BANK_SHIFT_GL = 10;
    // 12 x 1K RAM banks = 12K
    static const uint16_t RAM_BANKS_GL  = 12;
    static const uint32_t RAM_SIZE_GL   = (uint32_t)RAM_BANKS_GL << BANK_SHIFT_GL;

    // Optional initial RAM contents carried in a 6K image (else null)
    uint8_t* myInitialRAM;

    // Whether the PROM window ($1FC0-$1FDF) is currently enabled
    bool myEnablePROM;

    // The page access originally installed at the PROM window, restored when
    // PROM is disabled
    System::PageAccess myOrgAccess;

  private:
    // Following constructors and assignment operators not supported
    CartridgeGL();
    CartridgeGL(const CartridgeGL&);
    CartridgeGL& operator=(const CartridgeGL&);
};

#endif
