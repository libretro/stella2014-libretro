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
// $Id: Array.hxx 2838 2014-01-17 23:34:03Z stephena $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef ARRAY_HXX
#define ARRAY_HXX

#include "bspf.hxx"

namespace Common {

template <class T>
class Array
{
  protected:
    uint32_t _capacity;
    uint32_t _size;
    T *_data;

  public:
    typedef T *iterator;
    typedef const T *const_iterator;

  public:
    Array() : _capacity(0), _size(0), _data(0) {}
    Array(const Array<T>& array) : _capacity(0), _size(0), _data(0)
    {
      _size = array._size;
      _capacity = _size + 128;
      _data = new T[_capacity];
      for(uint32_t i = 0; i < _size; i++)
        _data[i] = array._data[i];
    }

    ~Array()
    {
      if (_data)
        delete [] _data;
    }

    void reserve(uint32_t capacity)
    {
      if(capacity <= _capacity)
        return;
      ensureCapacity(capacity - 128);
    }

    void push_back(const T& element)
    {
      ensureCapacity(_size + 1);
      _data[_size++] = element;
    }

    void push_back(const Array<T>& array)
    {
      ensureCapacity(_size + array._size);
      for(uint32_t i = 0; i < array._size; i++)
        _data[_size++] = array._data[i];
    }

    void insert_at(uint32_t idx, const T& element)
    {
      ensureCapacity(_size + 1);
      // The following loop is not efficient if you can just memcpy things around.
      // e.g. if you have a list of ints. But for real objects (String...), memcpy
      // usually isn't correct (specifically, for any class which has a non-default
      // copy behaviour. E.g. the String class uses a refCounter which has to be
      // updated whenever a String is copied.
      for(uint32_t i = _size; i > idx; i--)
        _data[i] = _data[i-1];

      _data[idx] = element;
      _size++;
    }

    T remove_at(uint32_t idx)
    {
      T tmp = _data[idx];
      for(uint32_t i = idx; i < _size - 1; i++)
        _data[i] = _data[i+1];
      _size--;
      return tmp;
    }

    T& operator [](uint32_t idx)
    {
      return _data[idx];
    }

    const T& operator [](uint32_t idx) const
    {
      return _data[idx];
    }

    Array<T>& operator =(const Array<T>& array)
    {
      if (_data)
        delete [] _data;
      _size = array._size;
      _capacity = _size + 128;
      _data = new T[_capacity];
      for(uint32_t i = 0; i < _size; i++)
        _data[i] = array._data[i];

      return *this;
    }

    uint32_t size() const     { return _size;     }
    uint32_t capacity() const { return _capacity; }

    void clear(bool fullerase = true)
    {
      if(fullerase)
      {
        if(_data)
        {
          delete [] _data;
          _data = 0;
        }
        _capacity = 0;
      }
      _size = 0;
    }
	
    bool isEmpty() const
    {
      return (_size == 0);
    }

    iterator begin()
    {
      return _data;
    }

    iterator end()
    {
      return _data + _size;
    }

    const_iterator begin() const
    {
      return _data;
    }

    const_iterator end() const
    {
      return _data + _size;
    }

  protected:
    void ensureCapacity(uint32_t new_len)
    {
      if (new_len <= _capacity)
        return;

      T *old_data = _data;
      _capacity = new_len + 128;
      _data = new T[_capacity];

      if (old_data)
      {
        // Copy old data
        for (uint32_t i = 0; i < _size; i++)
          _data[i] = old_data[i];
        delete [] old_data;
      }
    }
};

}  // Namespace Common

typedef Common::Array<int>   IntArray;
typedef Common::Array<bool>  BoolArray;
typedef Common::Array<uint8_t> ByteArray;

#endif
