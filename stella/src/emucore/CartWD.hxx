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
#include "CartEnhanced.hxx"

/**
  Cartridge class used for a Wickstead Design prototype ("Pink Panther").
  The 8K image is broken into eight 1K banks; selecting a bank arranges four
  of them (per a fixed table) into the four 1K segments of the 4K space. The
  low segment additionally overlays 64 bytes of RAM (read $1000-$103F, write
  $1040-$107F). Bankswitching is triggered by a read in $0030-$003F but takes
  effect a few cycles later (a pending-bank latch).

  Reimplemented on CartridgeEnhanced. The hotspot region is TIA space; reads
  and writes there are forwarded to the device that originally owned the page.

  @author  Thomas Jentzsch (original); 2014 port
*/
class CartridgeWD : public CartridgeEnhanced
{
  friend class CartridgeWDWidget;

  public:
    CartridgeWD(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~CartridgeWD();

  public:
    void reset();
    void install(System& system);

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

    bool save(Serializer& out) const;
    bool load(Serializer& in);

    string name() const { return "CartridgeWD"; }

  protected:
    bool checkSwitchBank(uint16_t, uint8_t) { return false; }

    uint16_t hotspot() const { return 0x0030; }

    bool bank(uint16_t bank, uint16_t segment);
    bool bank(uint16_t bank_) { return this->bank(bank_, 0); }

  private:
    // Cycle at which a (pending) bankswitch was initiated
    uint32_t myCyclesAtBankswitchInit;

    // The bank to switch to once the pending delay elapses
    uint16_t myPendingBank;

    // The currently active (logical) bank
    uint16_t myCurrentBank;

    // The page that originally owned the hotspot region ($0030); accesses
    // there are forwarded to it (the TIA)
    System::PageAccess myHotSpotPageAccess;

    // The arrangement of 1K banks mapped into the four segments per logical
    // bank
    struct BankOrg { uint8_t zero, one, two, three; };
    static const BankOrg ourBankOrg[8];

    static const uint16_t BANK_SHIFT_WD = 10;  // 1K segments
    static const uint32_t RAM_SIZE_WD   = 0x40;

  private:
    // Following constructors and assignment operators not supported
    CartridgeWD();
    CartridgeWD(const CartridgeWD&);
    CartridgeWD& operator=(const CartridgeWD&);
};

#endif
