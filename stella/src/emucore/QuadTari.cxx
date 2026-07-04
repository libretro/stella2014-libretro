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

#include "QuadTari.hxx"
#include "Joystick.hxx"
#include "System.hxx"
#include "TIA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTari::QuadTari(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::QuadTari),
    myFirstController(0),
    mySecondController(0)
{
  // For now the sub-controllers are always Joysticks (the common
  // four-player case). Both share this QuadTari's jack; the 'second' flag
  // routes the second sub-controller to the player 3/4 events.
  myFirstController  = new ::Joystick(jack, event, system, false);
  mySecondController = new ::Joystick(jack, event, system, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTari::~QuadTari()
{
  delete myFirstController;   myFirstController  = 0;
  delete mySecondController;  mySecondController = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::isFirst() const
{
  // If bit 7 of VBLANK is not set, the first controller is selected;
  // otherwise the second. The console toggles this line to multiplex the
  // two controllers, potentially several times per frame.
  return !(mySystem.tia().registerVBLANK() & 0x80);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::read(DigitalPin pin)
{
  // Override Controller::read(): the QuadTari can switch the active
  // controller multiple times per frame, so the select must be evaluated
  // on every read rather than latched once per update().
  if(isFirst())
    return myFirstController->read(pin);
  else
    return mySecondController->read(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTari::write(DigitalPin pin, bool value)
{
  if(isFirst())
    myFirstController->write(pin, value);
  else
    mySecondController->write(pin, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTari::update()
{
  myFirstController->update();
  mySecondController->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string QuadTari::name() const
{
  return "QT(" + myFirstController->name() + "/" +
         mySecondController->name() + ")";
}
