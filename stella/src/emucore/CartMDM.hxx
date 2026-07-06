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
#include "CartEnhanced.hxx"

/**
  Cartridge class used for "Menu Driven Megacart" (Edwin Blink). The image is
  a number of 4K banks; a read or write in $0800-$0BFF selects a bank from the
  low byte of the address. Selecting a bank above 127 permanently disables
  further switching (this is how the menu hands control to the chosen game).

  Reimplemented on CartridgeEnhanced. The hotspots live in the low mirror
  region and forward their underlying access, so (as in the previous
  standalone version) switching happens on both reads and writes.

  @author  Stephen Anthony (original); 2014 port
*/
class CartridgeMDM : public CartridgeEnhanced
{
  friend class CartridgeMDMWidget;

  public:
    CartridgeMDM(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeMDM();

  public:
    void install(System& system);

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

    bool save(Serializer& out) const;
    bool load(Serializer& in);

    string name() const { return "CartridgeMDM"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x0800; }

    bool bank(uint16_t bank, uint16_t segment);
    bool bank(uint16_t bank_) { return this->bank(bank_, 0); }

  private:
    // The pages containing the hotspots ($800-$F00); accesses are forwarded
    // to the devices that originally owned them.
    System::PageAccess myHotSpotPageAccess[8];

    // After a bank above 127 is selected, all further switching is disabled
    bool myBankingDisabled;

  private:
    // Following constructors and assignment operators not supported
    CartridgeMDM();
    CartridgeMDM(const CartridgeMDM&);
    CartridgeMDM& operator=(const CartridgeMDM&);
};

#endif
