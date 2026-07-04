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
// $Id: TIA.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef TIA_HXX
#define TIA_HXX

class Console;
class Settings;
class Sound;

#include "bspf.hxx"
#include "Device.hxx"
#include "System.hxx"
#include "TIATables.hxx"

/**
  This class is a device that emulates the Television Interface Adaptor 
  found in the Atari 2600 and 7800 consoles.  The Television Interface 
  Adaptor is an integrated circuit designed to interface between an 
  eight bit microprocessor and a television video modulator. It converts 
  eight bit parallel data into serial outputs for the color, luminosity, 
  and composite sync required by a video modulator.  

  This class outputs the serial data into a frame buffer which can then
  be displayed on screen.

  @author  Bradford W. Mott
  @version $Id: TIA.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class TIA : public Device
{
  public:
    friend class TIADebug;
    friend class RiotDebug;

    /**
      Create a new TIA for the specified console

      @param console  The console the TIA is associated with
      @param sound    The sound object the TIA is associated with
      @param settings The settings object for this TIA device
    */
    TIA(Console& console, Sound& sound, Settings& settings);
 
    /**
      Destructor
    */
    virtual ~TIA();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      Reset frame to current YStart/Height properties
    */
    void frameReset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset();

    /**
      Install TIA in the specified system.  Invoked by the system
      when the TIA is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system);

    /**
      Install TIA in the specified system and device.  Invoked by
      the system when the TIA is attached to it.  All devices
      which invoke this method take responsibility for chaining
      requests back to *this* device.

      @param system The system the device should install itself in
      @param device The device responsible for this address space
    */
    void install(System& system, Device& device);

    /**
      Save the current state of this device to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of this device from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const { return "TIA"; }

    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    uint8_t peek(uint16_t address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address

      @return  True if the poke changed the device address space, else false
    */
    bool poke(uint16_t address, uint8_t value);

    /**
      This method should be called at an interval corresponding to the 
      desired frame rate to update the TIA.  Invoking this method will update
      the graphics buffer and generate the corresponding audio samples.
    */
    void update();

    /**
      Answers the current frame buffer

      @return Pointer to the current frame buffer
    */
    uint8_t* currentFrameBuffer() const
      { return myCurrentFrameBuffer + myFramePointerOffset; }

    /**
      Answers the previous frame buffer

      @return Pointer to the previous frame buffer
    */
    uint8_t* previousFrameBuffer() const
      { return myPreviousFrameBuffer + myFramePointerOffset; }

    /**
      Answers the width and height of the frame buffer
    */
    inline uint32_t width() const  { return 160;           }
    inline uint32_t height() const { return myFrameHeight; }
    inline uint32_t ystart() const { return myFrameYStart; }

    /**
      Changes the current Height/YStart properties.
      Note that calls to these method(s) must be eventually followed by
      ::frameReset() for the changes to take effect.
    */
    void setHeight(uint32_t height) { myFrameHeight = height; }
    void setYStart(uint32_t ystart) { myFrameYStart = ystart; }

    /**
      Enables/disables auto-frame calculation.  If enabled, the TIA
      re-adjusts the framerate at regular intervals.

      @param mode  Whether to enable or disable all auto-frame calculation
    */
    void enableAutoFrame(bool mode) { myAutoFrameEnabled = mode; }

    /**
      Enables/disables color-loss for PAL modes only.

      @param mode  Whether to enable or disable PAL color-loss mode
    */
    void enableColorLoss(bool mode);

    /**
      Answers whether this TIA runs at NTSC or PAL scanrates,
      based on how many frames of out the total count are PAL frames.
    */
    bool isPAL()
      { return (uint64_t)myPALFrameCounter * 12 >= (uint64_t)myFrameCounter * 5; }

    uint64_t getMilliSeconds() const {
        // NTSC frames at 60 fps last 1000/60 = 50/3 ms; PAL at 50 fps, 20 ms
        uint64_t ntscFrames = myFrameCounter - myPALFrameCounter;
        return (ntscFrames * 50) / 3 + (uint64_t)myPALFrameCounter * 20;
    }

    /**
      Answers the current color clock we've gotten to on this scanline.

      @return The current color clock
    */
    uint32_t clocksThisLine() const
      { return ((mySystem->cycles() * 3) - myClockWhenFrameStarted) % 228; }

    /**
      Answers the scanline at which the current frame began drawing.

      @return The starting scanline
    */
    uint32_t startLine() const
      { return myStartScanline; }

    /**
      Answers the total number of scanlines the TIA generated in producing
      the current frame buffer. For partial frames, this will be the
      current scanline.

      @return The total number of scanlines generated
    */
    uint32_t scanlines() const
      { return ((mySystem->cycles() * 3) - myClockWhenFrameStarted) / 228; }

    /**
      Answers whether the TIA is currently in 'partial frame' mode
      (we're in between a call of startFrame and endFrame).

      @return If we're in partial frame mode
    */
    bool partialFrame() const { return myPartialFrameFlag; }

    /**
      Answers the current VBLANK register value. The QuadTari controller
      multiplexes two sub-controllers per jack and selects between them
      using VBLANK bit 7 (the "dump to ground" line the console toggles).
    */
    uint8_t registerVBLANK() const { return myVBLANK; }

    /**
      Answers the first scanline at which drawing occured in the last frame.

      @return The starting scanline
    */
    uint32_t startScanline() const { return myStartScanline; }

    /**
      Answers the current position of the virtual 'electron beam' used to
      draw the TIA image.  If not in partial frame mode, the position is
      defined to be in the lower right corner (@ width/height of the screen).
      Note that the coordinates are with respect to currentFrameBuffer(),
      taking any YStart values into account.

      @return The x/y coordinates of the scanline electron beam, and whether
              it is in the visible/viewable area of the screen
    */
    bool scanlinePos(uint16_t& x, uint16_t& y) const;

    /**
      Enables/disable/toggle the specified (or all) TIA bit(s).  Note that
      disabling a graphical object also disables its collisions.

      @param mode  1/0 indicates on/off, and values greater than 1 mean
                   flip the bit from its current state

      @return  Whether the bit was enabled or disabled
    */
    bool toggleBit(TIABit b, uint8_t mode = 2);
    bool toggleBits();

    /**
      Enables/disable/toggle the specified (or all) TIA bit collision(s).

      @param mode  1/0 indicates on/off, and values greater than 1 mean
                   flip the collision from its current state

      @return  Whether the collision was enabled or disabled
    */
    bool toggleCollision(TIABit b, uint8_t mode = 2);
    bool toggleCollisions();

    /**
      Toggle the display of HMOVE blanks.

      @return  Whether the HMOVE blanking was enabled or disabled
    */
    bool toggleHMOVEBlank();

    /**
      Enables/disable/toggle 'fixed debug colors' mode.

      @param mode  1/0 indicates on/off, otherwise flip from
                   its current state

      @return  Whether the mode was enabled or disabled
    */
    bool toggleFixedColors(uint8_t mode = 2);

    /**
      Enable/disable/query state of 'undriven/floating TIA pins'.

      @param mode  1/0 indicates on/off, otherwise return the current state

      @return  Whether the mode was enabled or disabled
    */
    bool driveUnusedPinsRandom(uint8_t mode = 2);

  private:
    /**
      Enables/disables all TIABit bits.  Note that disabling a graphical
      object also disables its collisions.

      @param mode  Whether to enable or disable all bits
    */
    void enableBits(bool mode);

    /**
      Enables/disables all TIABit collisions.

      @param mode  Whether to enable or disable all collisions
    */
    void enableCollisions(bool mode);

    // Update the current frame buffer to the specified color clock
    void updateFrame(int32_t clock);

    // Waste cycles until the current scanline is finished
    void waitHorizontalSync();

    // Reset horizontal sync counter
    void waitHorizontalRSync();

    // Clear both internal TIA buffers to black (palette color 0)
    void clearBuffers();

    // Set up bookkeeping for the next frame
    void startFrame();

    // Update bookkeeping at end of frame
    void endFrame();

    // Convert resistance from ports to dumped value
    uint8_t dumpedInputPort(int resistance);

    // Write the specified value to the HMOVE registers at the given clock
    void pokeHMP0(uint8_t value, int32_t clock);
    void pokeHMP1(uint8_t value, int32_t clock);
    void pokeHMM0(uint8_t value, int32_t clock);
    void pokeHMM1(uint8_t value, int32_t clock);
    void pokeHMBL(uint8_t value, int32_t clock);

    // Apply motion to registers when HMOVE is currently active
    void applyActiveHMOVEMotion(int hpos, int16_t& pos, int32_t motionClock);

    // Apply motion to registers when HMOVE was previously active
    void applyPreviousHMOVEMotion(int hpos, int16_t& pos, uint8_t motion);

  private:
    // Console the TIA is associated with
    Console& myConsole;

    // Sound object the TIA is associated with
    Sound& mySound;

    // Settings object the TIA is associated with
    Settings& mySettings;

    // Pointer to the current frame buffer
    uint8_t* myCurrentFrameBuffer;

    // Pointer to the previous frame buffer
    uint8_t* myPreviousFrameBuffer;

    // Pointer to the next pixel that will be drawn in the current frame buffer
    uint8_t* myFramePointer;

    // Indicates offset used by the exported frame buffer
    // (the exported frame buffer is a vertical 'sliding window' of the actual buffer)
    uint32_t myFramePointerOffset;

    // Indicates the number of 'colour clocks' offset from the base
    // frame buffer pointer
    // (this is used when loading state files with a 'partial' frame)
    uint32_t myFramePointerClocks;

    // Indicated what scanline the frame should start being drawn at
    uint32_t myFrameYStart;

    // Indicates the height of the frame in scanlines
    uint32_t myFrameHeight;

    // Indicates offset in color clocks when display should stop
    uint32_t myStopDisplayOffset;

    // Indicates color clocks when the current frame began
    int32_t myClockWhenFrameStarted;

    // Indicates color clocks when frame should begin to be drawn
    int32_t myClockStartDisplay;

    // Indicates color clocks when frame should stop being drawn
    int32_t myClockStopDisplay;

    // Indicates color clocks when the frame was last updated
    int32_t myClockAtLastUpdate;

    // Indicates how many color clocks remain until the end of 
    // current scanline.  This value is valid during the 
    // displayed portion of the frame.
    int32_t myClocksToEndOfScanLine;

    // Indicates the total number of scanlines generated by the last frame
    uint32_t myScanlineCountForLastFrame;

    // Indicates the maximum number of scanlines to be generated for a frame
    uint32_t myMaximumNumberOfScanlines;

    // Indicates potentially the first scanline at which drawing occurs
    uint32_t myStartScanline;

    // Color clock when VSYNC ending causes a new frame to be started
    int32_t myVSYNCFinishClock; 

    uint8_t myVSYNC;        // Holds the VSYNC register value
    uint8_t myVBLANK;       // Holds the VBLANK register value

    uint8_t myNUSIZ0;       // Number and size of player 0 and missle 0
    uint8_t myNUSIZ1;       // Number and size of player 1 and missle 1

    uint8_t myPlayfieldPriorityAndScore;
    uint8_t myPriorityEncoder[2][256];
    uint8_t myColor[8];
    uint8_t myFixedColor[8];
    uint8_t* myColorPtr;

    uint8_t myCTRLPF;       // Playfield control register

    bool myREFP0;         // Indicates if player 0 is being reflected
    bool myREFP1;         // Indicates if player 1 is being reflected

    uint32_t myPF;          // Playfield graphics (19-12:PF2 11-4:PF1 3-0:PF0)

    uint8_t myGRP0;         // Player 0 graphics register
    uint8_t myGRP1;         // Player 1 graphics register
    
    uint8_t myDGRP0;        // Player 0 delayed graphics register
    uint8_t myDGRP1;        // Player 1 delayed graphics register

    bool myENAM0;         // Indicates if missle 0 is enabled
    bool myENAM1;         // Indicates if missle 1 is enabled

    bool myENABL;         // Indicates if the ball is enabled
    bool myDENABL;        // Indicates if the vertically delayed ball is enabled

    uint8_t myHMP0;         // Player 0 horizontal motion register
    uint8_t myHMP1;         // Player 1 horizontal motion register
    uint8_t myHMM0;         // Missle 0 horizontal motion register
    uint8_t myHMM1;         // Missle 1 horizontal motion register
    uint8_t myHMBL;         // Ball horizontal motion register

    bool myVDELP0;        // Indicates if player 0 is being vertically delayed
    bool myVDELP1;        // Indicates if player 1 is being vertically delayed
    bool myVDELBL;        // Indicates if the ball is being vertically delayed

    bool myRESMP0;        // Indicates if missle 0 is reset to player 0
    bool myRESMP1;        // Indicates if missle 1 is reset to player 1

    uint16_t myCollision;     // Collision register

    // Determines whether specified collisions are enabled or disabled
    // The lower 16 bits are and'ed with the collision register to mask out
    // any collisions we don't want to be processed
    // The upper 16 bits are used to store which objects is currently
    // enabled or disabled
    // This is necessary since there are 15 collision combinations which
    // are controlled by 6 objects
    uint32_t myCollisionEnabledMask;

    // Note that these position registers contain the color clock 
    // on which the object's serial output should begin (0 to 159)
    int16_t myPOSP0;        // Player 0 position register
    int16_t myPOSP1;        // Player 1 position register
    int16_t myPOSM0;        // Missle 0 position register
    int16_t myPOSM1;        // Missle 1 position register
    int16_t myPOSBL;        // Ball position register

    // The color clocks elapsed so far for each of the graphical objects,
    // as denoted by 'MOTCK' line described in A. Towers TIA Hardware Notes
    int32_t myMotionClockP0;
    int32_t myMotionClockP1;
    int32_t myMotionClockM0;
    int32_t myMotionClockM1;
    int32_t myMotionClockBL;

    // Indicates 'start' signal for each of the graphical objects as
    // described in A. Towers TIA Hardware Notes
    int32_t myStartP0;
    int32_t myStartP1;
    int32_t myStartM0;
    int32_t myStartM1;

    // Index into the player mask arrays indicating whether display
    // of the first copy should be suppressed
    uint8_t mySuppressP0;
    uint8_t mySuppressP1;

    // Latches for 'more motion required' as described in A. Towers TIA
    // Hardware Notes
    bool myHMP0mmr;
    bool myHMP1mmr;
    bool myHMM0mmr;
    bool myHMM1mmr;
    bool myHMBLmmr;

    // Graphics for Player 0 that should be displayed.  This will be
    // reflected if the player is being reflected.
    uint8_t myCurrentGRP0;

    // Graphics for Player 1 that should be displayed.  This will be
    // reflected if the player is being reflected.
    uint8_t myCurrentGRP1;

    // It's VERY important that the BL, M0, M1, P0 and P1 current
    // mask pointers are always on a uint32_t boundary.  Otherwise,
    // the TIA code will fail on a good number of CPUs.
    const uint8_t* myP0Mask;
    const uint8_t* myM0Mask;
    const uint8_t* myM1Mask;
    const uint8_t* myP1Mask;
    const uint8_t* myBLMask;
    const uint32_t* myPFMask;

    // Audio values; only used by TIADebug
    uint8_t myAUDV0, myAUDV1, myAUDC0, myAUDC1, myAUDF0, myAUDF1;

    // Indicates when the dump for paddles was last set
    int32_t myDumpDisabledCycle;

    // Indicates if the dump is current enabled for the paddles
    bool myDumpEnabled;

    // Latches for INPT4 and INPT5
    uint8_t myINPT4, myINPT5;

    // Indicates if HMOVE blanks are currently or previously enabled,
    // and at which horizontal position the HMOVE was initiated
    int32_t myCurrentHMOVEPos;
    int32_t myPreviousHMOVEPos;
    bool myHMOVEBlankEnabled;
    bool myAllowHMOVEBlanks;

    // Indicates if unused TIA pins are randomly driven high or low
    // Otherwise, they take on the value previously on the databus
    bool myTIAPinsDriven;

    // Bitmap of the objects that should be considered while drawing
    uint8_t myEnabledObjects;

    // Determines whether specified bits (from TIABit) are enabled or disabled
    // This is and'ed with the enabled objects each scanline to mask out any
    // objects we don't want to be processed
    uint8_t myDisabledObjects;

    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL (and maybe SECAM) systems when the previous frame
    // contains an odd number of scanlines.
    bool myColorLossEnabled;

    // Indicates whether we're done with the current frame. poke() clears this
    // when VSYNC is strobed or the max scanlines/frame limit is hit.
    bool myPartialFrameFlag;

    // Automatic framerate correction based on number of scanlines
    bool myAutoFrameEnabled;

    // Number of total frames displayed by this TIA
    uint32_t myFrameCounter;

    // Number of PAL frames displayed by this TIA
    uint32_t myPALFrameCounter;

    // The framerate currently in use by the Console


    // Whether TIA bits/collisions are currently enabled/disabled
    bool myBitsEnabled, myCollisionsEnabled;

  private:
    // Copy constructor isn't supported by this class so make it private
    TIA(const TIA&);

    // Assignment operator isn't supported by this class so make it private
    TIA& operator = (const TIA&);
};

#endif
