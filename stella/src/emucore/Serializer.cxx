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
// $Id: Serializer.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <fstream>
#include <stdexcept>

#include "Serializer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer(const string& filename, bool readonly)
  : myStream(NULL),
    myUseFilestream(true)
{
  if(readonly)
  {
    //FilesystemNode node(filename);
    //if(node.isFile() && node.isReadable())
    {
      fstream* str = new fstream(filename.c_str(), ios::in | ios::binary);
      if(str && str->is_open())
      {
        myStream = str;
        myStream->exceptions( ios_base::failbit | ios_base::badbit | ios_base::eofbit );
        reset();
      }
      else
        delete str;
    }
  }
  else
  {
    // When using fstreams, we need to manually create the file first
    // if we want to use it in read/write mode, since it won't be created
    // if it doesn't already exist
    // However, if it *does* exist, we don't want to overwrite it
    // So we open in write and append mode - the write creates the file
    // when necessary, and the append doesn't delete any data if it
    // already exists
    fstream temp(filename.c_str(), ios::out | ios::app);
    temp.close();

    fstream* str = new fstream(filename.c_str(), ios::in | ios::out | ios::binary);
    if(str && str->is_open())
    {
      myStream = str;
      myStream->exceptions( ios_base::failbit | ios_base::badbit | ios_base::eofbit );
      reset();
    }
    else
      delete str;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::Serializer(void)
  : myStream(NULL),
    myUseFilestream(false)
{
  myStream = new stringstream(ios::in | ios::out | ios::binary);
  
  // For some reason, Windows and possibly OSX needs to store something in
  // the stream before it is used for the first time
  if(myStream)
  {
    myStream->exceptions( ios_base::failbit | ios_base::badbit | ios_base::eofbit );
    putBool(true);
    reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Serializer::~Serializer(void)
{
  if(myStream != NULL)
  {
    if(myUseFilestream)
      ((fstream*)myStream)->close();

    delete myStream;
    myStream = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::isValid(void)
{
  return myStream != NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::reset(void)
{
  myStream->clear();
  myStream->seekg(ios_base::beg);
  myStream->seekp(ios_base::beg);
}

uint32_t Serializer::size(void)
{
  if(myStream == NULL)
    return 0;

  // Temporarily disable exceptions so seeking to the end (and the
  // implicit EOF handling) doesn't throw
  ios_base::iostate mask = myStream->exceptions();
  myStream->exceptions(ios_base::goodbit);

  myStream->clear();
  streampos cur = myStream->tellg();
  myStream->seekg(0, ios_base::end);
  streampos end = myStream->tellg();
  myStream->clear();
  myStream->seekg(cur);

  myStream->exceptions(mask);

  return (end > 0) ? (uint32_t)end : 0;
}

void Serializer::setPosition(uint32_t pos)
{
  myStream->clear();
  myStream->seekg((streampos)pos);
  myStream->seekp((streampos)pos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t Serializer::getByte(void)
{
  char buf;
  myStream->read(&buf, 1);

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::getByteArray(uint8_t* array, uint32_t size)
{
  myStream->read((char*)array, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint16_t Serializer::getShort(void)
{
  uint16_t val = 0;
  myStream->read((char*)&val, sizeof(uint16_t));

  return val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::getShortArray(uint16_t* array, uint32_t size)
{
  myStream->read((char*)array, sizeof(uint16_t)*size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t Serializer::getInt(void)
{
  uint32_t val = 0;
  myStream->read((char*)&val, sizeof(uint32_t));

  return val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::getIntArray(uint32_t* array, uint32_t size)
{
  myStream->read((char*)array, sizeof(uint32_t)*size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Serializer::getString(void)
{
  int len = getInt();

  // A malformed or truncated stream can yield a bogus length here (a
  // negative value, or one far larger than the data that remains).
  // Passing that straight to string::resize()/read() means a giant
  // allocation or a long read that only fails at the very end. Instead,
  // reject any length that cannot possibly be satisfied by the bytes
  // left in the stream, so a bad state is refused immediately.
  stringstream* s = (stringstream*)myStream;
  streampos cur = s->tellg();
  s->seekg(0, ios::end);
  streampos end = s->tellg();
  s->seekg(cur, ios::beg);
  if(len < 0 || (streamoff)len > (end - cur))
    throw runtime_error("Serializer: invalid string length");

  string str;
  str.resize(len);
  myStream->read(&str[0], len);

  return str;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Serializer::getBool(void)
{
  return getByte() == TruePattern;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putByte(uint8_t value)
{
  myStream->write((char*)&value, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putByteArray(const uint8_t* array, uint32_t size)
{
  myStream->write((char*)array, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putShort(uint16_t value)
{
  myStream->write((char*)&value, sizeof(uint16_t));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putShortArray(const uint16_t* array, uint32_t size)
{
  myStream->write((char*)array, sizeof(uint16_t)*size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putInt(uint32_t value)
{
  myStream->write((char*)&value, sizeof(uint32_t));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putIntArray(const uint32_t* array, uint32_t size)
{
  myStream->write((char*)array, sizeof(uint32_t)*size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putString(const string& str)
{
  int len = str.length();
  putInt(len);
  myStream->write(str.data(), len);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Serializer::putBool(bool b)
{
  putByte(b ? TruePattern: FalsePattern);
}
