#ifndef _SYMBOL_HPP
#define _SYMBOL_HPP

#include "../carmel/src/weight.h"

#ifdef TEST
#include "test.hpp"
#endif


template <class C,class Alloc=std::allocator<C>,int block_size=4096 > class PackedAlloc {
  Alloc alloc;
  typedef list<C *> List;
  List blocks;
  C *free_start;
  C *free_end;
  C * allocate(size_t n) {
	if ( free_end - free_start >= n) {
	  C *ret=free_start;
	  free_start+=n;
	  return ret;
	} else {
	  C *ret;
	  if (n > block_size)  // one-off unique block just big enough
		ret=alloc.allocate(n);
	  else { // standard sized block with room to spare
	    ret=alloc.allocate(block_size);
		free_start = ret + n;
		free_end = ret + block_size;
	  }
	  blocks.push_back(ret);
  	  return ret;
	}
  }
  void deallocate(C *p, size_t n) {}
  PackedAlloc() {
	free_start=free_end=0;
  }
  ~PackedAlloc() {
	for (List::const_iterator i=blocks.begin(),end=blocks.end();i!=end;++i)
	  alloc.deallocate(*i);
  }
};

template <class C,class Alloc=std::allocator<C>,int block_size=4096 > class StaticPackedAlloc {
  static PackedAlloc<C,Alloc,block_size> alloc;
  C * allocate(size_t n) {
	return alloc.allocate();
  }
  void deallocate(C *p, size_t n) {}
);



template <class Alloc=StaticPackedAlloc<char> >
class StringInterner {
  Alloc alloc;
  //typedef hash_map<StringKey,StringKey> Table;
  typedef hash_set<StringKey> Table;
  Table interns;
  char *operator (const char *string) {
	StringKey s(string);
	pair<Table::local_iterator,bool> i=interns.insert(Table::value_type(s));
	return i->str;
	if (table.find(s) != table.end())
	  return *s;
	else {
	  table.insert(s);
	  return *(table.find(s));
	}
	// horrible ... insert should return pair<iterator,bool> but MS is naughty
  }
};

// uses interned char *
struct Symbol {
  static PackedAlloc<char> alloc;
  char *str;
};



#ifdef TEST
BOOST_AUTO_UNIT_TEST( SYMBOL)
{
}
#endif

#endif
