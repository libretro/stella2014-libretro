//============================================================================
//
//  BBBBB    SSSS   PPPPP   FFFFFF
//  BB  BB  SS  SS  PP  PP  FF
//  BB  BB  SS      PP  PP  FF
//  BBBBB    SSSS   PPPPP   FFFF    --  "Brad's Simple Portability Framework"
//  BB  BB      SS  PP      FF
//  BB  BB  SS  SS  PP      FF
//  BBBBB    SSSS   PP      FF
//
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: bspf.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott
  @version $Id: bspf.hxx 2838 2014-01-17 23:34:03Z stephena $
*/

#include <stdint.h>

// Types for 8-bit signed and unsigned integers
typedef int8_t Int8;
typedef uint8_t uInt8;
// Types for 16-bit signed and unsigned integers
typedef int16_t Int16;
typedef uint16_t uInt16;
// Types for 32-bit signed and unsigned integers
typedef int32_t Int32;
typedef uint32_t uInt32;
// Types for 64-bit signed and unsigned integers
typedef int64_t Int64;
typedef uint64_t uInt64;


// The following code should provide access to the standard C++ objects and
// types: string, ostream, istream, etc.
#include <algorithm>
#include <iomanip>
#include <string>
#include <cstring>
#include <cctype>
using namespace std;

static const string EmptyString("");

//////////////////////////////////////////////////////////////////////
// Some convenience functions

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

// Compare two strings, ignoring case
inline int BSPF_compareIgnoreCase(const string& s1, const string& s2)
{
#ifdef _MSC_VER
  return _stricmp(s1.c_str(), s2.c_str());
#else
  return strcasecmp(s1.c_str(), s2.c_str());
#endif
}
inline int BSPF_compareIgnoreCase(const char* s1, const char* s2)
{
#ifdef _MSC_VER
  return _stricmp(s1, s2);
#else
  return strcasecmp(s1, s2);
#endif
}

// Test whether the first string starts with the second one (case insensitive)
inline bool BSPF_startsWithIgnoreCase(const string& s1, const string& s2)
{
#ifdef _MSC_VER
  return _strnicmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
#else
  return strncasecmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
#endif
}
inline bool BSPF_startsWithIgnoreCase(const char* s1, const char* s2)
{
#ifdef _MSC_VER
  return _strnicmp(s1, s2, strlen(s2)) == 0;
#else
  return strncasecmp(s1, s2, strlen(s2)) == 0;
#endif
}

// Test whether two strings are equal (case insensitive)
inline bool BSPF_equalsIgnoreCase(const string& s1, const string& s2)
{
  return BSPF_compareIgnoreCase(s1, s2) == 0;
}

#endif
