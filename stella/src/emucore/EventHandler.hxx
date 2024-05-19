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
// $Id: EventHandler.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <map>

class Console;
class OSystem;
class DialogContainer;
class EventMappingWidget;
class MouseControl;
class StringList;
class VariantList;

#include "Array.hxx"
#include "Event.hxx"
#include "StellaKeys.hxx"
#include "bspf.hxx"

enum MouseButton {
  EVENT_LBUTTONDOWN,
  EVENT_LBUTTONUP,
  EVENT_RBUTTONDOWN,
  EVENT_RBUTTONUP,
  EVENT_WHEELDOWN,
  EVENT_WHEELUP
};

enum JoyHat {
  EVENT_HATUP     = 0,  // make sure these are set correctly,
  EVENT_HATDOWN   = 1,  // since they'll be used as array indices
  EVENT_HATLEFT   = 2,
  EVENT_HATRIGHT  = 3,
  EVENT_HATCENTER = 4
};

enum EventMode {
  kEmulationMode = 0,  // make sure these are set correctly,
  kMenuMode      = 1,  // since they'll be used as array indices
  kNumModes      = 2
};

/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontend will send translated events here, and the handler will
  check to see what the current 'mode' is.

  If in emulation mode, events received from the frontend are remapped and
  sent to the emulation core.  If in menu mode, the events are sent
  unchanged to the menu class, where (among other things) changing key
  mapping can take place.

  @author  Stephen Anthony
  @version $Id: EventHandler.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler(OSystem* osystem);
 
    /**
      Destructor
    */
    virtual ~EventHandler();

    // Enumeration representing the different states of operation
    enum State {
      S_NONE,
      S_EMULATE,
      S_PAUSE
    };

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    Event& event() { return myEvent; }

    /**
      Initialize state of this eventhandler.
    */
    void initialize();

    /**
      Resets the state machine of the EventHandler to the defaults

      @param state  The current state to set
    */
    void reset(State state);

    /**
      Joystick emulates 'impossible' directions (ie, left & right
      at the same time)

      @param allow  Whether or not to allow impossible directions
    */
    void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }

  private:
    // Global Event object
    Event myEvent;

    // Indicates whether the joystick emulates 'impossible' directions
    bool myAllowAllDirectionsFlag;;

    /**
      The following methods take care of assigning action mappings.
    */
    void setMouseAsPaddle(int paddle, const string& message);
};

#endif
