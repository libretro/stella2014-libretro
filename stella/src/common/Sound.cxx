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
// $Id: Sound.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <sstream>
#include <cmath>

#include "TIASnd.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "Sound.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::Sound(OSystem* osystem)
  : myIsEnabled(false),
    myIsInitializedFlag(false),
    myLastRegisterSetCycle(0),
    myNumChannels(0),
    myIsMuted(true),
    myVolume(100)
{
  myIsInitializedFlag = true;
  myOSystem           = osystem;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::~Sound()
{
  if(myIsInitializedFlag)
    myIsEnabled = myIsInitializedFlag = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::setEnabled(bool state)
{
  myOSystem->settings().setValue("sound", state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::open()
{
  myIsEnabled = false;
  mute(true);
  if(!myIsInitializedFlag || !myOSystem->settings().getBool("sound"))
  {
    return;
  }

  // Now initialize the TIASound object which will actually generate sound
  myTIASound.outputFrequency(31400);
  myTIASound.channels(2, myNumChannels == 2);

  // Adjust volume to that defined in settings
  myVolume = myOSystem->settings().getInt("volume");
  setVolume(myVolume);

  myIsEnabled = true;
  mute(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::close()
{
  if(myIsInitializedFlag)
  {
    myIsEnabled = false;
    myLastRegisterSetCycle = 0;
    myTIASound.reset();
    myRegWriteQueue.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::mute(bool state)
{
  if(myIsInitializedFlag)
    myIsMuted = state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::reset()
{
  if(myIsInitializedFlag)
  {
    myLastRegisterSetCycle = 0;
    myTIASound.reset();
    myRegWriteQueue.clear();
    mute(myIsMuted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::setVolume(Int32 percent)
{
  if(myIsInitializedFlag && (percent >= 0) && (percent <= 100))
  {
    myOSystem->settings().setValue("volume", percent);
    myVolume = percent;
    myTIASound.volume(percent);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::adjustVolume(Int8 direction)
{
  Int32 percent = myVolume;

  if(direction == -1)
    percent -= 2;
  else if(direction == 1)
    percent += 2;

  if((percent < 0) || (percent > 100))
    return;

  setVolume(percent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::adjustCycleCounter(Int32 amount)
{
  myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::setChannels(uInt32 channels)
{
  if(channels == 1 || channels == 2)
    myNumChannels = channels;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::set(uInt16 addr, uInt8 value, Int32 cycle)
{
  // Record how many CPU cycles have passed since the last register write.
  // All queue timing is integer CPU cycles: the TIA emits exactly one
  // audio sample every 38 CPU cycles (2 samples per 76-cycle scanline,
  // per real hardware / MiSTer RTL), so no seconds conversion is needed
  // and the arithmetic is exact and deterministic.
  Int32 delta = cycle - myLastRegisterSetCycle;
  if(delta < 0)
    delta = 0;

  RegWrite info;
  info.addr = addr;
  info.value = value;
  info.deltaCycles = (uInt32)delta;
  myRegWriteQueue.enqueue(info);

  // Update last cycle counter to the current cycle
  myLastRegisterSetCycle = cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::processFragment(Int16* stream, uInt32 length)
{
  // All timing below is in integer CPU cycles. The TIA produces exactly
  // one audio sample every 38 CPU cycles (2 samples per 76-cycle scanline,
  // as on real hardware and in the MiSTer FPGA implementation), so a
  // fragment of 'length' samples spans exactly length*38 cycles. This
  // arithmetic is exact and bit-for-bit deterministic on every platform.
  const uInt32 channels = 2;
  const uInt32 CYCLES_PER_SAMPLE = 38;

  uInt64 streamLengthCycles = (uInt64)length * CYCLES_PER_SAMPLE;
  uInt64 queuedCycles = myRegWriteQueue.durationCycles();
  if(queuedCycles > streamLengthCycles)
  {
    uInt64 excessCycles = queuedCycles - streamLengthCycles;
    uInt64 removedCycles = 0;
    while(removedCycles < excessCycles)
    {
      RegWrite& info = myRegWriteQueue.front();
      removedCycles += info.deltaCycles;
      myTIASound.set(info.addr, info.value);
      myRegWriteQueue.dequeue();
    }
  }

  uInt64 positionCycles = 0;   // elapsed time within this fragment

  while(positionCycles < streamLengthCycles)
  {
    uInt32 positionSamples = (uInt32)(positionCycles / CYCLES_PER_SAMPLE);

    if(myRegWriteQueue.size() == 0)
    {
      // There are no more pending TIA sound register updates so we'll
      // use the current settings to finish filling the sound fragment
      myTIASound.process(stream + (positionSamples * channels),
          length - positionSamples);

      // Since we had to fill the fragment we'll reset the cycle counter
      // to zero.  NOTE: This isn't 100% correct, however, it'll do for
      // now.  We should really remember the overrun and remove it from
      // the delta of the next write.
      myLastRegisterSetCycle = 0;
      break;
    }
    else
    {
      // There are pending TIA sound register updates so we need to
      // update the sound buffer to the point of the next register update
      RegWrite& info = myRegWriteQueue.front();

      // Cycles remaining until the end of this fragment
      uInt64 remainingCycles = streamLengthCycles - positionCycles;

      // Does the register update occur before the end of the fragment?
      if((uInt64)info.deltaCycles <= remainingCycles)
      {
        // If the register update time hasn't already passed then
        // process samples upto the point where it should occur
        if(info.deltaCycles > 0)
        {
          // Process the fragment up to the next TIA register write.
          // The sample count is the number of whole sample boundaries
          // crossed: floor((pos+delta)/38) - floor(pos/38).
          uInt32 nextSamples = (uInt32)
              ((positionCycles + info.deltaCycles) / CYCLES_PER_SAMPLE);
          myTIASound.process(stream + (positionSamples * channels),
              nextSamples - positionSamples);

          positionCycles += info.deltaCycles;
        }
        myTIASound.set(info.addr, info.value);
        myRegWriteQueue.dequeue();
      }
      else
      {
        // The next register update occurs in the next fragment so finish
        // this fragment with the current TIA settings and reduce the register
        // update delay by the corresponding amount of time
        myTIASound.process(stream + (positionSamples * channels),
            length - positionSamples);
        info.deltaCycles -= (uInt32)remainingCycles;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Sound::save(Serializer& out) const
{
   out.putString(name());

   uInt8 reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0, reg6 = 0;

   // Only get the TIA sound registers if sound is enabled
   if(myIsInitializedFlag)
   {
      reg1 = myTIASound.get(0x15);
      reg2 = myTIASound.get(0x16);
      reg3 = myTIASound.get(0x17);
      reg4 = myTIASound.get(0x18);
      reg5 = myTIASound.get(0x19);
      reg6 = myTIASound.get(0x1a);
   }

   out.putByte(reg1);
   out.putByte(reg2);
   out.putByte(reg3);
   out.putByte(reg4);
   out.putByte(reg5);
   out.putByte(reg6);

   out.putInt(myLastRegisterSetCycle);

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Sound::load(Serializer& in)
{
   if(in.getString() != name())
      return false;

   uInt8 reg1 = in.getByte(),
         reg2 = in.getByte(),
         reg3 = in.getByte(),
         reg4 = in.getByte(),
         reg5 = in.getByte(),
         reg6 = in.getByte();

   myLastRegisterSetCycle = (Int32) in.getInt();

   // Only update the TIA sound registers if sound is enabled
   // Make sure to empty the queue of previous sound fragments
   if(myIsInitializedFlag)
   {
      myRegWriteQueue.clear();
      myTIASound.set(0x15, reg1);
      myTIASound.set(0x16, reg2);
      myTIASound.set(0x17, reg3);
      myTIASound.set(0x18, reg4);
      myTIASound.set(0x19, reg5);
      myTIASound.set(0x1a, reg6);
   }

   return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::RegWriteQueue::RegWriteQueue(uInt32 capacity)
  : myCapacity(capacity),
    myBuffer(0),
    mySize(0),
    myHead(0),
    myTail(0)
{
  myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 Sound::RegWriteQueue::durationCycles()
{
  uInt64 duration = 0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].deltaCycles;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
    grow();

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::RegWrite& Sound::RegWriteQueue::front()
{
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Sound::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::RegWriteQueue::grow()
{
  RegWrite* buffer = new RegWrite[myCapacity * 2];
  for(uInt32 i = 0; i < mySize; ++i)
  {
    buffer[i] = myBuffer[(myHead + i) % myCapacity];
  }
  myHead = 0;
  myTail = mySize;
  myCapacity = myCapacity * 2;
  delete[] myBuffer;
  myBuffer = buffer;
}
