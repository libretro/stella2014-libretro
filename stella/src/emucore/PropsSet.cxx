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
// $Id: PropsSet.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <map>

#include "bspf.hxx"

#include "DefProps.hxx"
#include "OSystem.hxx"
#include "Props.hxx"
#include "Settings.hxx"

#include "PropsSet.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::PropertiesSet(OSystem* osystem)
  : myOSystem(osystem)
{
  //load(myOSystem->propertiesFile());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertiesSet::~PropertiesSet()
{
  myExternalProps.clear();
  myTempProps.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PropertiesSet::getMD5(const string& md5, Properties& properties,
                           bool useDefaults) const
{
  properties.setDefaults();
  bool found = false;

  // There are three lists to search when looking for a properties entry,
  // which must be done in the following order
  // If 'useDefaults' is specified, only use the built-in list
  //
  //  'save': entries previously inserted that are saved on program exit
  //  'temp': entries previously inserted that are discarded
  //  'builtin': the defaults compiled into the program

  // First check properties from external file
  if(!useDefaults)
  {
    // Check external list
    PropsList::const_iterator iter = myExternalProps.find(md5);
    if(iter != myExternalProps.end())
    {
      properties = iter->second;
      found = true;
    }
    else  // Search temp list
    {
      iter = myTempProps.find(md5);
      if(iter != myTempProps.end())
      {
        properties = iter->second;
        found = true;
      }
    }
  }

  // Otherwise, search the internal database using binary search
  if(!found)
  {
    int low = 0, high = DEF_PROPS_SIZE - 1;
    while(low <= high)
    {
      int i = (low + high) / 2;
      int cmp = BSPF_compareIgnoreCase(md5, DefProps[i][Cartridge_MD5]);

      if(cmp == 0)  // found it
      {
        for(int p = 0; p < LastPropType; ++p)
          if(DefProps[i][p][0] != 0)
            properties.set((PropertyType)p, DefProps[i][p]);

        found = true;
        break;
      }
      else if(cmp < 0)
        high = i - 1; // look at lower range
      else
        low = i + 1;  // look at upper range
    }
  }

  return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::insert(const Properties& properties, bool save)
{
  // Note that the following code is optimized for insertion when an item
  // doesn't already exist, and when the external properties file is
  // relatively small (which is the case with current versions of Stella,
  // as the properties are built-in)
  // If an item does exist, it will be removed and insertion done again
  // This shouldn't be a speed issue, as insertions will only fail with
  // duplicates when you're changing the current ROM properties, which
  // most people tend not to do

  // Since the PropSet is keyed by md5, we can't insert without a valid one
  const string& md5 = properties.get(Cartridge_MD5);
  if(md5 == "")
    return;

  // The status of 'save' determines which list to save to
  PropsList& list = save ? myExternalProps : myTempProps;

  pair<PropsList::iterator,bool> ret;
  ret = list.insert(make_pair(md5, properties));
  if(ret.second == false)
  {
    // Remove old item and insert again
    list.erase(ret.first);
    list.insert(make_pair(md5, properties));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PropertiesSet::removeMD5(const string& md5)
{
  // We only remove from the external list
  myExternalProps.erase(md5);
}
