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

#ifndef CARTRIDGE3EX_HXX
#define CARTRIDGE3EX_HXX

#include "bspf.hxx"
#include "Cart3E.hxx"

/**
  Enhanced version of 3E which supports up to 256KB RAM. It is otherwise
  identical to 3E; the only difference is that the number of RAM banks is
  read from the ROM ($FFFA holds "RAM bank count - 1") rather than fixed.

  @author  Thomas Jentzsch (original); reimplemented on CartridgeEnhanced
           for the 2014 core
*/
class Cartridge3EX : public Cartridge3E
{
  friend class Cartridge3EXWidget;

  public:
    Cartridge3EX(const uint8_t* image, uint32_t size, const Settings& settings);
    virtual ~Cartridge3EX();

  public:
    string name() const { return "Cartridge3EX"; }

  private:
    // Following constructors and assignment operators not supported
    Cartridge3EX();
    Cartridge3EX(const Cartridge3EX&);
    Cartridge3EX& operator=(const Cartridge3EX&);
};

#endif
