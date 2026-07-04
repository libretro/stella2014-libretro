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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "System.hxx"
#include "Serializer.hxx"
#include "CartARM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARM::CartridgeARM(const Settings& settings)
  : Cartridge(settings),
    myThumbEmulator(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARM::~CartridgeARM()
{
#ifdef THUMB_SUPPORT
  delete myThumbEmulator;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::createThumbulator(const uint16_t* rom, uint16_t* ram)
{
#ifdef THUMB_SUPPORT
  delete myThumbEmulator;
  myThumbEmulator = new Thumbulator;
  thumb_init(myThumbEmulator, rom, ram,
             mySettings.getBool("thumb.trapfatal"));
  // Default to NTSC timing; setArmTimingForFormat() refines this once the
  // console's display format is known.
  thumb_set_timer_rate(myThumbEmulator, 42000000u, 715909u);
#else
  (void)rom; (void)ram;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::setArmTimingForFormat(const string& format)
{
#ifdef THUMB_SUPPORT
  // Exact integer ARM-ticks-per-6507-cycle ratios (70 MHz ARM vs the
  // 6507 clock): NTSC 42000000/715909, PAL 3500000/59069, SECAM 1120/19.
  if(myThumbEmulator == 0)
    return;

  if(format == "PAL" || format == "PAL60")
    thumb_set_timer_rate(myThumbEmulator, 3500000u, 59069u);
  else if(format == "SECAM" || format == "SECAM60")
    thumb_set_timer_rate(myThumbEmulator, 1120u, 19u);
  else
    thumb_set_timer_rate(myThumbEmulator, 42000000u, 715909u);
#else
  (void)format;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::runArm(uint32_t cycles, bool irqDrivenAudio)
{
#ifdef THUMB_SUPPORT
  if(myThumbEmulator != 0)
    thumb_run_cycles(myThumbEmulator, cycles, irqDrivenAudio ? 1 : 0);
#else
  (void)cycles; (void)irqDrivenAudio;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::updateCycles(int cycles)
{
  // The 2014 core does not feed ARM cycles back into the 6507 clock (ARM
  // code runs in zero 6507 cycles), so nothing to do here. Kept so the
  // concrete mappers can call it at the same points as in Stella 7.
  (void)cycles;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeARM::saveArmState(Serializer& out) const
{
#ifdef THUMB_SUPPORT
  try
  {
    if(myThumbEmulator != 0)
    {
      // Persist the timer state so run-ahead / savestates stay in sync
      // with the ARM audio pacing.
      out.putInt(myThumbEmulator->t1_tcr);
      out.putInt(myThumbEmulator->t1_tc);
      out.putInt(myThumbEmulator->timer_frac);
    }
    else
    {
      out.putInt(0);
      out.putInt(0);
      out.putInt(0);
    }
  }
  catch(...)
  {
    return false;
  }
#else
  (void)out;
#endif
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeARM::loadArmState(Serializer& in)
{
#ifdef THUMB_SUPPORT
  try
  {
    uint32_t tcr = in.getInt();
    uint32_t tc  = in.getInt();
    uint32_t frac = in.getInt();
    if(myThumbEmulator != 0)
    {
      myThumbEmulator->t1_tcr = tcr;
      myThumbEmulator->t1_tc = tc;
      myThumbEmulator->timer_frac = frac;
    }
  }
  catch(...)
  {
    return false;
  }
#else
  (void)in;
#endif
  return true;
}
