//two things, really: parsing for Tree Transducer symbols, and an intern (that is, canonical pointer for all equivalent strings) facility
#ifndef SYMBOL_HPP
#define SYMBOL_HPP
// no separate implementation for now, just #define MAIN in one source file that includes this

//#include "weight.h"
#include <graehl/shared/2hash.h>
#include <graehl/shared/stringkey.h>
#include <graehl/shared/list.h>
#include <graehl/shared/charbuf.hpp>
#include <iostream>
#include <graehl/shared/genio.h>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <graehl/shared/tree.hpp>
#endif

#include <graehl/shared/packedalloc.hpp>

#ifndef MIN_LEGAL_ADDRESS
//FIXME: portability issue:
// for debugging purposes - printing union of pointer/small int - output will be WRONG if any heap memory is allocated below this:
#define MIN_LEGAL_ADDRESS ((void *)0x1000)
#endif

namespace graehl {

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
    typename Table::insert_result_type it;
    if ( (it = interns.insert(typename Table::value_type(s,Empty()))).second ) {
      char *s=alloc.allocate(strlen(string)+1);
      Assert(s >= MIN_LEGAL_ADDRESS);
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

#include <graehl/shared/static_itoa.h>

// uses interned char *
struct Symbol {
  static StringInterner<> intern;
  char *str;
  static Symbol empty;
  static Symbol ZERO;
  Symbol() : str(empty.str) {}
  explicit Symbol(unsigned i) : str(intern(static_utoa(i)))
  {
        //str=intern(static_itoa(i));
  }
  Symbol(unsigned i, bool dummy) {
    phony_int()=i;
  }
  Symbol(const char *s) : str(intern(s)) {}
  Symbol(const Symbol &s) : str(s.str) {}
  operator char * () { return str; }
  Symbol & operator=(Symbol r) {
    str=r.str;
    return *this;
  }
  enum make_not_anon_23 { PHONYINT };
  /*  static Symbol make_phony_int(unsigned i) {
    static Symbol s;
    s.phony_int() = i;
    return s;
    }*/
  /// just conversion between int and str ... phony ints can't be printed or read.  really just implements a union; hash and sort should work as long as you don't mix with non-phony Symbol
  unsigned &phony_int() {
    return *(unsigned *)&str;
  }
  unsigned phony_int() const {
    return *(const unsigned *)str;
  }

    char *c_str() const { return str; }
    bool operator < (const Symbol r) const // for Dinkum / MS .NET 2003 hash table (buckets sorted by key, takes an extra comparison since a single valued < is used rather than a 3 value strcmp
    {
        return str<r.str;//strcmp(str,r.str)<0;
    }
    ptrdiff_t cmp(const Symbol r) const {
        return r.str-str;
    }
    bool operator ==(const Symbol r) const {
    return r.str==str;
  }
  bool operator !=(const Symbol r) const {
    return r.str!=str;
  }
  bool isDefault() const {
        return *this == empty;
  }
  void makeDefault() {
        *this = empty;
  }
  // empty symbol (isDefault()) is considered an io failure
  template <class charT, class Traits>
  std::ios_base::iostate
  print(std::basic_ostream<charT,Traits>& o) const
  {
      //FIXME: dude, print it escaped the same way as it's read? (round trip was not intended originally)
    if (str < MIN_LEGAL_ADDRESS) {
      o << "symbolint_" << phony_int();
    } else {
        if (isDefault())
          return GENIOBAD;
        o << str;
    }
    return GENIOGOOD;
  }

  // either quoted strings (e.g. "a") with quotes in the string, and backslashes, escaped with a preceding backslash, e.g. "a\"\\\"b" (and the quotes and backslashes are considered part of the symbol, not equal to the unquoted version)
  // or sequences of characters except for space,tab,newline,comma,",`,(,),#,$,:,{,},;,... (see the switch stmt)
  // empty symbol (isDefault()) is considered an io failure
  template <class charT, class Traits>
  std::ios_base::iostate
  read(std::basic_istream<charT,Traits>& in)
  {
        g_buf.clear(); // FIXME: not threadsafe
        char c;
//      GENIO_CHECK(in>>c);
        EXPECTI_COMMENT_FIRST(in>>c);
          if (c=='"') {
                bool last_escape=false;
                g_buf.push_back(c);
                for(;;) {
                  EXPECTI(in.get(c));
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
                    case '(':case ')':case ',':case '"':case '`':case '=':
                    case '%':case '{':case '}':
                    case '\t':case '\r':case '\n':case ' ':
#ifndef GRAPHVIZ_SYMBOL
                    case '^':case ';':    //FIXME: think this breaks treeviz.cpp (but it's needed for transducer.hpp)
#endif
//case '#':
//case '$':
//case ':':
                        in.unget();
                        goto donewhile;
                    case '\\':
                        EXPECTI(in.get(c));
//fallthrough
                    default:
                        g_buf.push_back(c);
                    }
                } while (in.get(c));
          }

donewhile:
          if (g_buf.size()==0) {
fail:
                makeDefault();
                return GENIOBAD;
          }
        g_buf.push_back(0);
        str=intern(g_buf.begin());
    return GENIOGOOD;
  }

};

bool operator ==(const Symbol s,const char *c) {
  return s.operator==(c);
}
bool operator ==(const char *c,const Symbol s) {
  return s.operator==(c);
}

#include <graehl/shared/dynarray.h>
template <unsigned upto=32>
struct SmallIntSymbols {
  auto_array<Symbol> cache;
  SmallIntSymbols() : cache(upto) {
    for (unsigned i=0;i<upto;++i)
      new(cache.begin()+i) Symbol(i);
  }
  Symbol operator()(unsigned i) {
    if (i<upto)
      return cache[i];
    else
      return Symbol(i);
  }
};



#define SMALLINT 100

//extern SmallIntSymbols<SMALLINT> g_smallint;

CREATE_INSERTER(Symbol)
CREATE_EXTRACTOR(Symbol)

#ifdef GRAEHL__SINGLE_MAIN
//SmallIntSymbols<SMALLINT> g_smallint;
StringInterner<> Symbol::intern;
//const char * Symbol::str_empty=Symbol::intern("");
Symbol Symbol::empty("");
Symbol Symbol::ZERO(0,Symbol::PHONYINT);
#endif


#ifdef GRAEHL_TEST

char const* symbol_test_strs[]={"test string","d","el""abc","","e","fall","","e","very very long more than 8","a","b","e",0};



BOOST_AUTO_TEST_CASE( symbol )
{
    using namespace graehl;
    using namespace std;
  char buf[1000];
  {buf[0]='a';
  buf[1]=0;
  char *s="a";
//  DBP(Symbol(s)<<endl);
//  BOOST_CHECK(Symbol::borrow(s)==Symbol::borrow(buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  }
    {buf[0]='b';
  buf[1]=0;
  char *s="b";
  //DBP(Symbol(s)<<endl);
//  BOOST_CHECK(Symbol::borrow(s)==Symbol::borrow(buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  }
          {buf[0]='a';
  buf[1]=0;
  char *s="a";
//  DBP(Symbol(s)<<endl);
//  BOOST_CHECK(Symbol::borrow(s)==Symbol::borrow(buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  BOOST_CHECK(Symbol(s)==Symbol(buf));
  BOOST_CHECK(!strcmp(Symbol(s).str,buf));
  }
  char *last=0;
  for(char const**i=symbol_test_strs;*i;++i) {
    char const *a=*i;
    strcpy(buf,a);
    char *c=Symbol(a);
    char *d=Symbol(buf);
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
        std::string sa=" 1a  ";
        char *sb="1a";
        std::istringstream isa(sa);isa >> a;
        BOOST_CHECK(!strcmp(a.str,sb));
//      DBP('|'<<sa<<'|'<<a<<'|'<<sb<< '|'<<endl);
        BOOST_CHECK(a==Symbol(sb));
  }
   {
        Symbol a;
        std::string sa=" \"\\\"a\"  ";
        char *sb="\"\\\"a\"";
        std::istringstream isa(sa); isa >> a;
        BOOST_CHECK(!strcmp(a.str,sb));
        //DBP('|'<<sa<<'|'<<a<<'|'<<sb<< '|'<<endl);
        BOOST_CHECK(a==Symbol(sb));
  }
 {
  Tree<Symbol> a,b;
  std::string sa="1(2,3(aa,5,6))";
  std::string sb="1(2 3(aa 5 6))";
  std::stringstream o;
  std::istringstream isa(sa);isa >> a;
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
  std::string sa="1 ( 2 , 3 ( aa , 5 , 6 ) )";
  std::string sb="1(2 3(aa 5 6))";
  std::stringstream o;
  std::istringstream isa(sa);isa >> a;
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
  std::string sa="1as(\"2\\\"\",3(aa(),5,6))";
  std::string sb="1as(\"2\\\"\" 3(aa 5 6))";
  std::stringstream o;
  std::istringstream isa(sa);isa >> a;
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
 BOOST_CHECK(Symbol(1)==Symbol("1"));
 BOOST_CHECK(Symbol(91)=="91");
}
#endif

}

BEGIN_HASH_VAL(graehl::Symbol) {
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4311 )
#endif
    return uint32_hash(reinterpret_cast<std::size_t>(x.str)); //FIXME: probably 64-bit pointer unsafe (only uses sizeof(unsigned)-LSBytes)
#ifdef _MSC_VER
#pragma warning( pop )
#endif
} END_HASH


#endif
