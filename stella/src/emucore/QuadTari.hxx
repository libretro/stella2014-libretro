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

#ifndef QUADTARI_HXX
#define QUADTARI_HXX

#include "bspf.hxx"
#include "Control.hxx"

/**
  The QuadTari controller. It multiplexes two sub-controllers onto one
  physical jack, selecting between them with VBLANK bit 7, so a single jack
  presents two logical controllers -- allowing four-player games. This
  backport supports Joystick sub-controllers (the common four-player case);
  it can be extended to Driving/Paddles sub-controllers, which also exist
  in this tree.

  @author  Thomas Jentzsch (original); backported for the 2014 core
*/
class QuadTari : public Controller
{
  public:
    /**
      Create a QuadTari controller plugged into the specified jack.

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
    */
    QuadTari(Jack jack, const Event& event, const System& system);
    virtual ~QuadTari();

  public:
    using Controller::read;

    /**
      Read the value of the specified digital pin for this controller.
      The QuadTari can switch the active controller multiple times per
      frame, so the select is evaluated on every read rather than once
      in update().

      @param pin The pin of the controller jack to read
      @return The state of the pin
    */
    virtual bool read(DigitalPin pin);

    /**
      Write the given value to the specified digital pin.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    virtual void write(DigitalPin pin, bool value);

    /**
      Update the sub-controllers' state from the event structure.
    */
    virtual void update();

    /**
      Answers the name of this controller.
    */
    virtual string name() const;

  private:
    /**
      Answers whether the first (true) or second (false) sub-controller is
      currently selected, based on VBLANK bit 7.
    */
    bool isFirst() const;

    Controller* myFirstController;
    Controller* mySecondController;

    // Copying/assignment not supported
    QuadTari(const QuadTari&);
    QuadTari& operator = (const QuadTari&);
};

#endif
