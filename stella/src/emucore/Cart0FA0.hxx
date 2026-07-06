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
#include "CartEnhanced.hxx"

/**
  Cartridge class used for some Brazilian 8K bankswitched games. There are two
  4K banks, selected by accessing $x6A0 (bank 0) or $x6C0 (bank 1) in the
  TIA/RIOT mirror region; the hotspot pages forward the underlying access.

  Reimplemented on CartridgeEnhanced. The power-on bank is 0, preserving the
  behaviour of the previous standalone implementation.

  @author  Thomas Jentzsch (original); 2014 port
*/
class Cartridge0FA0 : public CartridgeEnhanced
{
  friend class Cartridge0FA0Widget;

  public:
    Cartridge0FA0(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~Cartridge0FA0();

  public:
    void install(System& system);

    uint8_t peek(uint16_t address);
    bool poke(uint16_t address, uint8_t value);

    string name() const { return "Cartridge0FA0"; }

  protected:
    bool checkSwitchBank(uint16_t address, uint8_t value);

    uint16_t hotspot() const { return 0x06a0; }

  private:
    // The page containing the hotspots; reads/writes are forwarded to the
    // device that originally owned it (the TIA/RIOT).
    System::PageAccess myHotSpotPageAccess;

  private:
    // Following constructors and assignment operators not supported
    Cartridge0FA0();
    Cartridge0FA0(const Cartridge0FA0&);
    Cartridge0FA0& operator=(const Cartridge0FA0&);
};

#endif
