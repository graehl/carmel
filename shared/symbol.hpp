#ifndef _SYMBOL_HPP
#define _SYMBOL_HPP
// no separate implementation for now, just #define MAIN in one source file that includes this

//#include "../carmel/src/weight.h"
#include "../carmel/src/stringkey.h"
#include "../carmel/src/list.h"
#include "charbuf.hpp"
#include <iostream>
#include "../carmel/src/genio.h"

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
    //  return i->first.str;

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
  static StringInterner<> intern;
  char *str;
  Symbol() : str(empty) {}
  Symbol(char *s) : str(intern(s)) {}
  Symbol(const Symbol &s) : str(s.str) {}
  static char * borrow(char *s) {
    return intern(s);
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

  template <class charT, class Traits>
  std::ios_base::iostate
  print_on(std::basic_ostream<charT,Traits>& o) const
  {
	o << str;
    return std::ios_base::goodbit;
  }

  // either quoted strings (e.g. "a") with quotes in the string, and backslashes, escaped with a preceding backslash, e.g. "a\"\\\"b" (and the quotes and backslashes are considered part of the symbol, not equal to the unquoted version)
  // or sequences of characters except for space,tab,newline,comma,",`,(,),#,$,:,{,}
  template <class charT, class Traits>
  std::ios_base::iostate
  get_from(std::basic_istream<charT,Traits>& in)
  {
	g_buf.clear();
	char c;
	GENIO_CHECK(in>>c);
	  if (c=='"') {
		bool last_escape=false;
		g_buf.push_back(c);
		for(;;) {
		  GENIO_CHECK_ELSE(in.get(c),str=empty);
		  g_buf.push_back(c); // even though we allow escapes/quotes, we treat them as part of the literal string
		  if (c=='"' && !last_escape)
			break;
		  if(c=='\\')
			last_escape=!last_escape;
		  else
		    last_escape=false;
		}
	  } else {
		do {
		  switch(c) {
case '(':case ')':case ',':case '"':case ' ':case '`':
case '#':case '$':case ':':case '{':case '}':
case '\t':case '\r':case '\n':
  in.putback(c);
  goto donewhile;
default:
  g_buf.push_back(c);
		  }
		} while (in.get(c));
	  }
	
donewhile:	
	g_buf.push_back(0);
	str=intern(g_buf.begin()); // don't use vector but instead use DynamicArray where we can guarantee contiguous array
    return std::ios_base::goodbit;
  }

};

CREATE_INSERTER(Symbol)
CREATE_EXTRACTOR(Symbol)

#ifdef MAIN
StringInterner<> Symbol::intern;
char * Symbol::empty = Symbol::intern("");
#endif

BEGIN_HASH_VAL(Symbol) {
  return (size_t)(x.str);
} END_HASH


#ifdef TEST

#include "tree.hpp"

char *symbol_test_strs[]={"test string","d","el""abc","","e","fall","","e","very very long more than 8","a","b","e",0};



BOOST_AUTO_UNIT_TEST( symbol )
{
  char buf[1000];
  {buf[0]='a';
  buf[1]=0;
  char *s="a";
  BOOST_CHECK(Symbol::borrow(s)==Symbol::borrow(buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  }
    {buf[0]='b';
  buf[1]=0;
  char *s="b";
  BOOST_CHECK(Symbol::borrow(s)==Symbol::borrow(buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  }
	  {buf[0]='a';
  buf[1]=0;
  char *s="a";
  BOOST_CHECK(Symbol::borrow(s)==Symbol::borrow(buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  }
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
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4800 )
#endif
      BOOST_CHECK(strcmp(last,a));
      BOOST_CHECK(strcmp(last,d));
#ifdef _MSC_VER
#pragma warning( pop )
#endif
    }
    last=d;
  }
  {
	Symbol a;
	string sa=" 1a  ";
	char *sb="1a";
	istringstream(sa) >> a;
	BOOST_CHECK(!strcmp(a.str,sb));
	BOOST_CHECK(a==Symbol(sa));
	DBP('|'<<sa<<'|'<<a<<'|'<<sb<< '|'<<endl);
	BOOST_CHECK(a==Symbol(sb));
  }
   {
	Symbol a;
	string sa=" \"\\\"a\"  ";
	char *sb="\"\\\"a\"";
	istringstream(sa) >> a;
	BOOST_CHECK(!strcmp(a.str,sb));
	DBP('|'<<sa<<'|'<<a<<'|'<<sb<< '|'<<endl);
	BOOST_CHECK(a==Symbol(sb));
  }
 {
  Tree<Symbol> a,b;
  string sa="1(2,3(aa,5,6))";
  string sb="1(2 3(aa 5 6))";
  stringstream o;
  istringstream(sa) >> a;
  o << a;
  BOOST_CHECK(o.str() == sb);
  o >> b;
  BOOST_CHECK(a == b);
  BOOST_CHECK(a.label == Symbol("1"));
  BOOST_REQUIRE(a.size()>1);
  BOOST_CHECK(a[0]->label == Symbol("2"));
  BOOST_REQUIRE(a.child(1)->size() > 1);
  BOOST_CHECK(a.child(1)->child(0)->label == Symbol("aa"));
 }
 {
  Tree<Symbol> a,b;
  string sa="1 ( 2 , 3 ( aa , 5 , 6 ) )";
  string sb="1(2 3(aa 5 6))";
  stringstream o;
  istringstream(sa) >> a;
  o << a;
  BOOST_CHECK(o.str() == sb);
  o >> b;
  BOOST_CHECK(a == b);
  BOOST_CHECK(a.label == Symbol("1"));
  BOOST_REQUIRE(a.size()>1);
  BOOST_CHECK(a[0]->label == Symbol("2"));
  BOOST_REQUIRE(a.child(1)->size() > 1);
  BOOST_CHECK(a.child(1)->child(0)->label == Symbol("aa"));
 }
 {
  Tree<Symbol> a,b;
  string sa="1as(\"2\\\"\",3(aa(),5,6))";
  string sb="1as(\"2\\\"\" 3(aa 5 6))";
  stringstream o;
  istringstream(sa) >> a;
  o << a;
  BOOST_CHECK(o.str() == sb);
  o >> b;
  BOOST_CHECK(a == b);
  BOOST_CHECK(a.label == Symbol("1as"));
  BOOST_REQUIRE(a.size()>1);
  BOOST_CHECK(a[0]->label == Symbol("\"2\\\"\""));
  BOOST_REQUIRE(a.child(1)->size() > 1);
  BOOST_CHECK(a.child(1)->child(0)->label == Symbol("aa"));
 }
}
#endif

#endif
