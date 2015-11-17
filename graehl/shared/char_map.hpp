// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef MAP_CHAR_2012524_HPP
#define MAP_CHAR_2012524_HPP

#include <limits.h>
#include <boost/ptr_container/ptr_array.hpp>
#include <locale>
#include <boost/config.hpp>

namespace graehl {

template <class V>
struct char_map
{
  BOOST_STATIC_CONSTANT(unsigned, size = UCHAR_MAX+1);   //std::ctype<char>::table_size
  V table[size];
  char_map() {}
  char_map(char_map const& o)
  {
    for (unsigned i = 0; i<size; ++i) table[i] = o[i];
  }
  char_map& operator = (char_map const& o)
  {
    for (unsigned i = 0; i<size; ++i) table[i] = o[i];
    return *this;
  }
  typedef V * iterator;
  typedef iterator const_iterator;
  iterator begin() const
  { return const_cast<V*>(table); }
  iterator end() const
  { return begin()+size; }

  V & operator[](char c)
  { return table[(unsigned char)c]; }
  V const& operator[](char c) const
  { return table[(unsigned char)c]; }
  V & operator[](unsigned c)
  { return table[c]; }
  V const& operator[](unsigned c) const
  { return table[c]; }
};

}


#endif
