#ifndef _SYMBOL_HPP
#define _SYMBOL_HPP
// no separate implementation for now, just #define MAIN in one source file that includes this

//#include "../carmel/src/weight.h"
#include "../carmel/src/stringkey.h"
#include "../carmel/src/list.h"

#ifdef TEST
#include "test.hpp"
#endif

#include "packedalloc.hpp"

template <class Alloc=StaticPackedAlloc<char> >
class StringInterner {
  Alloc alloc;
  //typedef hash_map<StringKey,StringKey> Table;
  struct Empty{};
  //FIXME: use unordered_set instead of Empty val
  typedef HashTable<StringKey,Empty> Table;
  Table interns;

public:
  char *operator ()(const char *string) {
	StringKey s((char *)string);
	//pair<Table::local_iterator,bool> i=interns.insert(Table::value_type(s,Empty()));
//	return i->first.str;

#if 1
		typename Table::insert_return_type it;
		if ( (it = interns.insert(typename Table::value_type(s,Empty()))).second ) {
		  char *s=alloc.allocate(strlen(string)+1); 
		  strcpy(s,string);
		  (const_cast<StringKey*>(&(it.first->first)))->str = s; // doesn't change hashval so ok
		  return s;
		} else
		  return it.first->first.str;
#else
	Table::local_iterator i=table.find(s)

	if ((i=table.find(s)) != table.end())
	  return i->first.str;
	else {
	  std::pair<StringKey,Empty> entry(alloc.allocate(strlen(string)+1),Empty());
	  strcpy(entry.first,string);
	  table.insert(entry);
	  return entry.first.str;
	}
#endif
  }
  StringInterner() {
	(*this)("");
  }
};

// uses interned char *
struct Symbol {
  static StringInterner<> interns;
  char *str;
  Symbol() : str(empty) {}
  Symbol(char *s) : str(interns(s)) {}
  Symbol(Symbol s) : str(s.str) {}
  static char * borrow(char *s) {
	return interns(s);
  }
  static char *giveBack(char *s) {
	// technically, leaks memory.  we could use StringPool instead.
  }
  static char *empty;
  Symbol & operator=(Symbol r) {
	str=r.str;
	return *this;
  }
  bool operator ==(Symbol r) const {
	return r.str==str;
  }
  bool operator !=(Symbol r) const {
	return r.str!=str;
  }

};

#ifdef MAIN
//PackedAlloc<char> StaticPackedAlloc<char>::alloc;
StringInterner<> Symbol::interns;
char * Symbol::empty = Symbol::interns("");
#endif

BEGIN_HASH_VAL(Symbol) {
 return (size_t)(x.str);
} END_HASH


#ifdef TEST
char *symbol_test_strs[]={"test string","d","el""abc","","e","fall","","e","very very long more than 8","a","b","e",0};

BOOST_AUTO_UNIT_TEST( SYMBOL)
{
  char buf[1000];
  char *last=0;
  for(char **i=symbol_test_strs;*i;++i) {
   char *a=*i;
   strcpy(buf,a);
   char *c=Symbol::borrow(a);
   char *d=Symbol::borrow(buf);
   BOOST_CHECK(!strcmp(a,c));
   BOOST_CHECK(!strcmp(a,d));
   BOOST_CHECK(!strcmp(a,c));
   BOOST_CHECK(c==d);
   if (last) {
	 BOOST_CHECK(last!=d);
	 BOOST_CHECK(strcmp(last,a)!=0);
	 BOOST_CHECK(strcmp(last,d)!=0);
   }
   last=d;
  }
}
#endif

#endif
