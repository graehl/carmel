#ifndef _DEBUGPRINT_HPP
#define _DEBUGPRINT_HPP

#include "threadlocal.hpp"
#include "myassert.h"
#include <sstream>
#include "byref.hpp"
#include <iostream>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include "genio.h"
#include "byref.hpp"


struct DebugWriter
{
  template <class charT, class Traits,class Label>
  std::basic_ostream<charT,Traits>&
  operator()(std::basic_ostream<charT,Traits>& o,const Label &l) const {
    dbgout(o,l);
    return o;
  }
};


template <class C,class V=void>
struct has_print_on;

template <class C,class V>
struct has_print_on {
};

template <class C>
struct has_print_on<C,typename C::has_print_on> {
  typedef void type;
};

template <class C>
struct has_print_on<boost::reference_wrapper<C> > : public has_print_on<C> {};



template <class C,class V=void>
struct has_print_on_writer;

template <class C,class V>
struct has_print_on_writer {
};

template <class C>
struct has_print_on_writer<C,typename C::has_print_on_writer> {
  typedef void type;
};

template <class C>
struct has_print_on_writer<boost::reference_wrapper<C> > : public has_print_on_writer<C> {};



template <class C,class V=void>
struct not_has_print_on_writer;

template <class C,class V>
struct not_has_print_on_writer {
  typedef void type;
};

template <class C>
struct not_has_print_on_writer<C,typename C::has_print_on_writer> {
};

template <class C>
struct not_has_print_on_writer<boost::reference_wrapper<C> > : public not_has_print_on_writer<C> {};


template <class C,class V=void,class V2=void>
struct has_print_on_plain;

template <class C,class V,class V2>
struct has_print_on_plain {
};

template <class C>
struct has_print_on_plain<C,typename not_has_print_on_writer<C>::type, typename has_print_on<C>::type> {
  typedef void type;
};

template <class C>
struct has_print_on_plain<boost::reference_wrapper<C> > : public has_print_on_plain<C> {};


/*template<class A>
static const char * dbgstr(const A &a);

template<class A,class Writer>
static const char * dbgstr(const A &a,Writer writer);
*/

template <class A>
void dbgout(ostream &o,const A a,typename enable_if<boost::is_pointer<A> >::type* dummy = 0) {
  if (a == NULL) {
    o << "NULL";
  } else {

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif
  o << "0x" << hex << (size_t)a << dec << "=&((";
#ifdef _MSC_VER
#pragma warning( pop )
#endif
  dbgout(o,*a);
  o << "))";
  }
}

template<class A>
void dbgout(ostream &o,const A &a, typename has_print_on_plain<A>::type* dummy = 0) {
  deref(a).print_on(o);
}

template<class A>
void dbgout(ostream &o,const A &a, typename has_print_on_writer<A>::type* dummy = 0) {
  deref(a).print_on(o,DebugWriter());
}


template<class A,class W>
void dbgout(ostream &o,const A &a,W w,typename has_print_on_plain<A>::type* dummy = 0) {
  deref(a).print_on(o);
}

template<class A,class W>
void dbgout(ostream &o,const A &a,W w,typename has_print_on_writer<A>::type* dummy = 0) {
  deref(a).print_on(o,w);
}



template <class A>
void dbgout(ostream &o,const A &a, typename enable_if<boost::is_arithmetic<A> >::type* dummy = 0) {
  o << a;
}

void dbgout(ostream &o,const char *a) {
  o << a;
}

void dbgout(ostream &o,const std::string &a) {
  o << a;
}



#ifdef DEBUG

#ifdef _MSC_VER
#include <windows.h>
#define DBPS(a) (OutputDebugString((const char *)(a)))
#else
#define DBPS(a) (Config::debug() << (a))
#endif

#include <boost/preprocessor/stringize.hpp>
#define DBPRE DBPS(__FILE__ "(" BOOST_PP_STRINGIZE(__LINE__) "):")
#define DBPOST DBPS("\n")
#define BDBP(a) do { DBPS(" " #a "=_<");DBPS(dbgstr(a));DBPS(">_"); } while(0)

#define DBP(a) do { DBPRE; BDBP(a);DBPOST; } while(0)
#define DBP2(a,b) do { DBPRE; BDBP(a); BDBP(b);DBPOST; } while(0)
#define DBP3(a,b,c) do { DBPRE; BDBP(a); BDBP(b); BDBP(c);DBPOST;} while(0)
//#define DBP2(a,p) do { DBPS(DBPRE(a,__FILE__,__LINE__)  #a " = ");DBPS(dbgstr(a,p));  } while(0)
//#define DBP(a) do { DBPS(DBPRE(a,__FILE__,__LINE__)  #a " = ");DBPS(dbgstr(a)); } while(0)

#define DBPC(c,a) do { DBPRE; DBPS(" (" c ")"); BDBP(a); DBPOST; } while(0)

#define BDBPW(a,w) do { DBPS(" " #a "=_<");DBPS(dbgstr(a,w));DBPS(">_"); } while(0)
#define DBPW(a,w) do { DBPRE; BDBPW(a,w) ;DBPOST; } while(0)


#else
#define DBP(a) 
#define DBP2(a,b) 
#define DBP3(a,b,c) 

#define DBPC(c,a) 

#define BDBPW(a,w) 
#define DBPW(a,w) 

#endif


static const string constEmptyString;
extern THREADLOCAL ostringstream dbgbuf;
extern THREADLOCAL string dbgstring;

#ifdef MAIN
THREADLOCAL ostringstream dbgbuf;
THREADLOCAL string dbgstring;
#endif



template<class A>
const char * dbgstr(const A &a) {
  dbgbuf.str(constEmptyString);
  //dbgbuf.clear(); // error flags
  dbgout(dbgbuf,a);
  Assert(dbgbuf.good());
  dbgstring=dbgbuf.str();
  return dbgstring.c_str(); //FIXME: since string is a copy by std, may need to use a static string as well
}

template<class A,class W>
const char * dbgstr(const A &a,W w) {
  dbgbuf.str(constEmptyString);
  //dbgbuf.clear(); // error flags
  dbgout(dbgbuf,a,w);
  Assert(dbgbuf.good());
  dbgstring=dbgbuf.str();
  return dbgstring.c_str(); //FIXME: since string is a copy by std, may need to use a static string as well
}

template<class A>
const char * dbgstrw(const A &a) {
  return dbgstr(a);
}

#endif
