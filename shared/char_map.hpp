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
  BOOST_STATIC_CONSTANT(unsigned,size=UCHAR_MAX+1);   //std::ctype<char>::table_size
  V table[size];
  char_map() {}
  char_map(char_map const& o)
  {
    for (unsigned i=0;i<size;++i) table[i]=o[i];
  }
  char_map& operator=(char_map const& o)
  {
    for (unsigned i=0;i<size;++i) table[i]=o[i];
    return *this;
  }
  typedef V * iterator;
  typedef iterator const_iterator;
//    typedef bool const* const_iterator;
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
