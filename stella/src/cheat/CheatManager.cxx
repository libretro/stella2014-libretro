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
// $Id: CheatManager.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <fstream>
#include <sstream>

#include "OSystem.hxx"
#include "Console.hxx"
#include "Cheat.hxx"
#include "Settings.hxx"
#include "CheetahCheat.hxx"
#include "BankRomCheat.hxx"
#include "RamCheat.hxx"

#include "CheatManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::CheatManager(OSystem* osystem)
  : myOSystem(osystem),
    myCurrentCheat(""),
    myListIsDirty(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::~CheatManager()
{
  myCheatMap.clear();
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Cheat* CheatManager::add(const string& name, const string& code,
                               bool enable, int idx)
{
  Cheat* cheat = (Cheat*) createCheat(name, code);
  if(!cheat)
    return NULL;

  // Delete duplicate entries
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    if(myCheatList[i]->name() == name || myCheatList[i]->code() == code)
    {
      myCheatList.remove_at(i);
      break;
    }
  }

  // Add the cheat to the main cheat list
  if(idx == -1)
    myCheatList.push_back(cheat);
  else
    myCheatList.insert_at(idx, cheat);

  // And enable/disable it (the cheat knows how to enable or disable itself)
  if(enable)
    cheat->enable();
  else
    cheat->disable();

  return cheat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::remove(int idx)
{
  if((unsigned int)idx >= myCheatList.size())
    return;

  Cheat* c = myCheatList[idx];

  // First remove it from the per-frame list
  addPerFrame(c, false);

  // Then remove it from the cheatlist entirely
  myCheatList.remove_at(idx);
  c->disable();
  delete c;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::addPerFrame(Cheat* cheat, bool enable)
{
  if(!cheat)
    return;

  // Make sure there are no duplicates
  bool found = false;
  unsigned int i;
  for(i = 0; i < myPerFrameList.size(); i++)
  {
    if(myPerFrameList[i]->code() == cheat->code())
    {
      found = true;
      break;
    }
  }

  if(enable)
  {
    if(!found)
      myPerFrameList.push_back(cheat);
  }
  else
  {
    if(found)
      myPerFrameList.remove_at(i);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::addOneShot(const string& name, const string& code)
{
  Cheat* cheat = (Cheat*) createCheat(name, code);
  if(!cheat)
    return;

  // Evaluate this cheat once, and then immediately delete it
  cheat->evaluate();
  delete cheat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Cheat* CheatManager::createCheat(const string& name, const string& code)
{
  if(!isValidCode(code))
    return NULL;

  // Create new cheat based on string length
  switch(code.size())
  {
    case 4:
      return new RamCheat(myOSystem, name, code);
      break;

    case 6:
      return new CheetahCheat(myOSystem, name, code);
      break;

    case 8:
      return new BankRomCheat(myOSystem, name, code);
      break;

    default:
      return NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::parse(const string& cheats)
{
  StringList s;
  string::size_type lastPos = cheats.find_first_not_of(",", 0);
  string::size_type pos     = cheats.find_first_of(",", lastPos);
  string cheat, name, code;

  // Split string by comma, getting each cheat
  while(string::npos != pos || string::npos != lastPos)
  {
    // Get the next cheat
    cheat = cheats.substr(lastPos, pos - lastPos);

    // Split cheat by colon, separating each part
    string::size_type lastColonPos = cheat.find_first_not_of(":", 0);
    string::size_type colonPos     = cheat.find_first_of(":", lastColonPos);
    while(string::npos != colonPos || string::npos != lastColonPos)
    {
      s.push_back(cheat.substr(lastColonPos, colonPos - lastColonPos));
      lastColonPos = cheat.find_first_not_of(":", colonPos);
      colonPos     = cheat.find_first_of(":", lastColonPos);
    }

    // Account for variable number of items specified for cheat
    switch(s.size())
    {
      case 1:
        name = s[0];
        code = name;
        add(name, code, true);
        break;

      case 2:
        name = s[0];
        code = s[1];
        add(name, code, true);
        break;

      case 3:
        name = s[0];
        code = s[1];
        add(name, code, s[2] == "1");
        break;
    }
    s.clear();

    lastPos = cheats.find_first_not_of(",", pos);
    pos     = cheats.find_first_of(",", lastPos);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::enable(const string& code, bool enable)
{
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    if(myCheatList[i]->code() == code)
    {
      if(enable)
        myCheatList[i]->enable();
      else
        myCheatList[i]->disable();
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::clear()
{
  // Don't delete the items from per-frame list, since it will be done in
  // the following loop
  myPerFrameList.clear();

  for(unsigned int i = 0; i < myCheatList.size(); i++)
    delete myCheatList[i];
  myCheatList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheatManager::isValidCode(const string& code)
{
  for(unsigned int i = 0; i < code.size(); i++)
    if(!isxdigit(code[i]))
      return false;

  int length = code.length();
  return (length == 4 || length == 6 || length == 8);
}
