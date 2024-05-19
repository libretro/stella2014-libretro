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
//
// $Id: FrameBuffer.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef FRAMEBUFFER_HXX
#define FRAMEBUFFER_HXX

#include "EventHandler.hxx"
class OSystem;

// Return values for initialization of framebuffer window
enum FBInitStatus {
  kSuccess,
  kFailComplete,
  kFailTooLarge,
  kFailNotSupported,
};


class FrameBuffer
{
  public:
    /**
      Creates a new Frame Buffer
    */
    FrameBuffer();

    /**
      Destructor
    */
    ~FrameBuffer();

    /**
      Set up the TIA/emulation palette for a screen of any depth > 8.

      @param palette  The array of colors
    */
    void setTIAPalette(const uInt32* palette);
};

#endif
