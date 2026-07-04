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
// $Id: Sound.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef SOUND_HXX
#define SOUND_HXX

class OSystem;

#include "Serializable.hxx"
#include "bspf.hxx"
#include "TIASnd.hxx"

/**
  This class is an abstract base class for the various sound objects.
  It has no functionality whatsoever.

  @author Stephen Anthony
  @version $Id: Sound.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class Sound : public Serializable
{
  public:
    /**
      Create a new sound object.  The init method must be invoked before
      using the object.
    */
    Sound(OSystem* osystem);

    /**
      Destructor
    */
    ~Sound();

    /**
      Enables/disables the sound subsystem.

      @param enable  Either true or false, to enable or disable the sound system
    */
    void setEnabled(bool enable);

    /**
      The system cycle counter is being adjusting by the specified amount.  Any
      members using the system cycle counter should be adjusted as needed.

      @param amount The amount the cycle counter is being adjusted by
    */
    void adjustCycleCounter(int32_t amount);

    /**
      Sets the number of channels (mono or stereo sound).

      @param channels The number of channels
    */
    void setChannels(uint32_t channels);

    /**
      Start the sound system, initializing it if necessary.  This must be
      called before any calls are made to derived methods.
    */
    void open();

    /**
      Should be called to stop the sound system.  Once called the sound
      device can be started again using the ::open() method.
    */
    void close();

    /**
      Set the mute state of the sound object.  While muted no sound is played.

      @param state Mutes sound if true, unmute if false
    */
    void mute(bool state);

    /**
      Reset the sound device.
    */
    void reset();

    /**
      Sets the sound register to a given value.

      @param addr  The register address
      @param value The value to save into the register
      @param cycle The system cycle at which the register is being updated
    */
    void set(uint16_t addr, uint8_t value, int32_t cycle);

    /**
      Sets the volume of the sound device to the specified level.  The
      volume is given as a percentage from 0 to 100.  Values outside
      this range indicate that the volume shouldn't be changed at all.

      @param percent The new volume percentage level for the sound device
    */
    void setVolume(int32_t percent);

    /**
      Adjusts the volume of the sound device based on the given direction.

      @param direction  Increase or decrease the current volume by a predefined
                        amount based on the direction (1 = increase, -1 =decrease)
    */
    void adjustVolume(int8_t direction);

    /**
      Invoked by the sound callback to process the next sound fragment.
      The stream is 16-bits (even though the callback is 8-bits), since
      the TIASnd class always generates signed 16-bit stereo samples.

      @param stream  Pointer to the start of the fragment
      @param length  Length of the fragment
    */
    void processFragment(int16_t* stream, uint32_t length);

    /**
      Saves the current state of this device to the given Serializer.

      @param out  The serializer device to save to.
      @return  The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out) const;

    /**
      Loads the current state of this device from the given Serializer.

      @param in  The Serializer device to load from.
      @return  The result of the load.  True on success, false on failure.
    */
    bool load(Serializer& in);

    /**
      Get a descriptor for this console class (used in error checking).

      @return  The name of the object
    */
    string name() const { return "TIASound"; }

  protected:
    // The OSystem for this sound object
    OSystem* myOSystem;

    // Struct to hold information regarding a TIA sound register write.
    // Timing is kept in integer CPU cycles for exactness/determinism:
    // the TIA produces exactly one audio sample every 38 CPU cycles
    // (2 samples per 76-cycle scanline, as on real hardware and in the
    // MiSTer FPGA implementation).
    struct RegWrite
    {
      uint16_t addr;
      uint8_t value;
      uint32_t deltaCycles;
    };

    /**
      A queue class used to hold TIA sound register writes before being
      processed while creating a sound fragment.
    */
    class RegWriteQueue
    {
      public:
        /**
          Create a new queue instance with the specified initial
          capacity.  If the queue ever reaches its capacity then it will
          automatically increase its size.
        */
        RegWriteQueue(uint32_t capacity = 512);

        /**
          Destroy this queue instance.
        */
        virtual ~RegWriteQueue();

        /**
          Clear any items stored in the queue.
        */
        void clear();

        /**
          Dequeue the first object in the queue.
        */
        void dequeue();

        /**
          Return the total duration of all the items in the queue,
          in integer CPU cycles.
        */
        uint64_t durationCycles();

        /**
          Enqueue the specified object.
        */
        void enqueue(const RegWrite& info);

        /**
          Return the i-th queued item counted from the front,
          for state serialization.
        */
        const RegWrite& peek(uint32_t i) const
          { return myBuffer[(myHead + i) % myCapacity]; }

        /**
          Return the item at the front on the queue.

          @return  The item at the front of the queue.
        */
        RegWrite& front();

        /**
          Answers the number of items currently in the queue.

          @return  The number of items in the queue.
        */
        uint32_t size() const;

      private:
        // Increase the size of the queue
        void grow();

        uint32_t myCapacity;
        RegWrite* myBuffer;
        uint32_t mySize;
        uint32_t myHead;
        uint32_t myTail;
    };

  private:
    // TIASound emulation object
    TIASound myTIASound;

    // Indicates if the sound subsystem is to be initialized
    bool myIsEnabled;

    // Indicates if the sound device was successfully initialized
    bool myIsInitializedFlag;

    // Indicates the cycle when a sound register was last set
    int32_t myLastRegisterSetCycle;

    // Indicates the number of channels (mono or stereo)
    uint32_t myNumChannels;

    // Indicates if the sound is currently muted
    bool myIsMuted;

    // Current volume as a percentage (0 - 100)
    uint32_t myVolume;

    // Queue of TIA register writes
    RegWriteQueue myRegWriteQueue;
};

#endif
