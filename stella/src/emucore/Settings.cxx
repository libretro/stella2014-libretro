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
// $Id: Settings.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <algorithm>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Version.hxx"

#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(OSystem* osystem)
  : myOSystem(osystem)
{
  // Add this settings object to the OSystem
  myOSystem->attach(this);

  // Add options that are common to all versions of Stella
  setInternal("video", "soft");

  // OpenGL specific options
  setInternal("gl_inter", "false");
  setInternal("gl_aspectn", "90");
  setInternal("gl_aspectp", "100");
  setInternal("gl_fsscale", "false");
  setInternal("gl_lib", "libGL.so");
  setInternal("gl_vsync", "true");
  setInternal("gl_vbo", "true");

  // Framebuffer-related options
  setInternal("tia_filter", "zoom2x");
  setInternal("fullscreen", "0");
  setInternal("fullres", "auto");
  setInternal("center", "false");
  setInternal("grabmouse", "true");
  setInternal("palette", "standard");
  setInternal("colorloss", "true");
  setInternal("timing", "sleep");
  setInternal("uimessages", "true");

  // TV filtering options
  setInternal("tv_filter", "0");
  setInternal("tv_scanlines", "25");
  setInternal("tv_scaninter", "true");
  // TV options when using 'custom' mode
  setInternal("tv_contrast", "0.0");
  setInternal("tv_brightness", "0.0");
  setInternal("tv_hue", "0.0");
  setInternal("tv_saturation", "0.0");
  setInternal("tv_gamma", "0.0");
  setInternal("tv_sharpness", "0.0");
  setInternal("tv_resolution", "0.0");
  setInternal("tv_artifacts", "0.0");
  setInternal("tv_fringing", "0.0");
  setInternal("tv_bleed", "0.0");

  // Sound options
  setInternal("sound", "true");
  setInternal("fragsize", "512");
  setInternal("freq", "31400");
  setInternal("volume", "100");

  // Input event options
  setInternal("keymap", "");
  setInternal("joymap", "");
  setInternal("combomap", "");
  setInternal("joydeadzone", "13");
  setInternal("joyallow4", "false");
  setInternal("usemouse", "analog");
  setInternal("dsense", "5");
  setInternal("msense", "7");
  setInternal("saport", "lr");
  setInternal("ctrlcombo", "true");

  // Snapshot options
  setInternal("snapsavedir", "");
  setInternal("snaploaddir", "");
  setInternal("snapname", "int");
  setInternal("sssingle", "false");
  setInternal("ss1x", "false");
  setInternal("ssinterval", "2");

  // Config files and paths
  setInternal("romdir", "");
  setInternal("statedir", "");
  setInternal("cheatfile", "");
  setInternal("palettefile", "");
  setInternal("propsfile", "");
  setInternal("nvramdir", "");
  setInternal("cfgdir", "");

  // ROM browser options
  setInternal("exitlauncher", "false");
  setInternal("launcherfont", "medium");
  setInternal("launcherexts", "allroms");
  setInternal("romviewer", "0");
  setInternal("lastrom", "");

  // UI-related options
  setInternal("uipalette", "0");
  setInternal("listdelay", "300");
  setInternal("mwheel", "4");

  // Misc options
  setInternal("autoslot", "false");
  setInternal("loglevel", "1");
  setInternal("logtoconsole", "0");
  setInternal("tiadriven", "false");
  setInternal("cpurandom", "true");
  setInternal("ramrandom", "true");
  setInternal("avoxport", "");
  setInternal("stats", "false");
  setInternal("fastscbios", "false");
  setExternal("romloadcount", "0");
  setExternal("maxres", "");

  // Debugger/disassembly options
  setInternal("dbg.fontstyle", "0");
  setInternal("dbg.uhex", "true");
  setInternal("dis.resolve", "true");
  setInternal("dis.gfxformat", "2");
  setInternal("dis.showaddr", "true");
  setInternal("dis.relocate", "false");

  // Thumb ARM emulation options
  setInternal("thumb.trapfatal", "true");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
  myInternalSettings.clear();
  myExternalSettings.clear();
}

const Variant& Settings::value(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return myInternalSettings[idx].value;
  else if((idx = getExternalPos(key)) != -1)
    return myExternalSettings[idx].value;
  else
    return EmptyVariant;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setValue(const string& key, const Variant& value)
{
  if(int idx = getInternalPos(key) != -1)
    setInternal(key, value, idx);
  else
    setExternal(key, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInternalPos(const string& key) const
{
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
    if(myInternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getExternalPos(const string& key) const
{
  for(unsigned int i = 0; i < myExternalSettings.size(); ++i)
    if(myExternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setInternal(const string& key, const Variant& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos != -1 && pos >= 0 && pos < (int)myInternalSettings.size() &&
     myInternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
    {
      if(myInternalSettings[i].key == key)
      {
        idx = i;
        break;
      }
    }
  }

  if(idx != -1)
  {
    myInternalSettings[idx].key   = key;
    myInternalSettings[idx].value = value;
    if(useAsInitial) myInternalSettings[idx].initialValue = value;
  }
  else
  {
    Setting setting;
    setting.key   = key;
    setting.value = value;
    if(useAsInitial) setting.initialValue = value;

    myInternalSettings.push_back(setting);
    idx = myInternalSettings.size() - 1;
  }

  return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setExternal(const string& key, const Variant& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos != -1 && pos >= 0 && pos < (int)myExternalSettings.size() &&
     myExternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(unsigned int i = 0; i < myExternalSettings.size(); ++i)
    {
      if(myExternalSettings[i].key == key)
      {
        idx = i;
        break;
      }
    }
  }

  if(idx != -1)
  {
    myExternalSettings[idx].key   = key;
    myExternalSettings[idx].value = value;
    if(useAsInitial) myExternalSettings[idx].initialValue = value;
  }
  else
  {
    Setting setting;
    setting.key   = key;
    setting.value = value;
    if(useAsInitial) setting.initialValue = value;

    myExternalSettings.push_back(setting);
    idx = myExternalSettings.size() - 1;
  }

  return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(const Settings&)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings& Settings::operator = (const Settings&)
{
  return *this;
}
