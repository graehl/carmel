#ifndef _DEBUGPRINT_HPP
#define _DEBUGPRINT_HPP

/// In your clase, define a type: if you defined a
///  print_on(ostream &) const
///   method like genio.h: GENIO_print_on
///  typedef void has_print_on;
/// or if has two arg print_on(ostream &, Writer &w)
///   typedef void has_print_on_writer;
/// if you inherit from a class that has defined one of these, override it with some other type than void: typedef bool has_print_on_writer would disable


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


// how this works: has_print_on<C>::type is either defined (iff C::has_print_on was typedefed to void), or undefined.  this allows us to exploit SFINAE (google) for function overloads depending on whether C::has_print_on was typedefed void ;)

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


// type is defined iff you're not a pointer type and you have no print_on(o) or print_on(o,writer)
template <class C,class V=void>
struct not_has_print_on;

template <class C,class V>
struct not_has_print_on {
  typedef void type;
};

template <class C>
struct not_has_print_on<C,typename has_print_on<C>::type> {
};

template <class C>
struct not_has_print_on<C,typename has_print_on_writer<C>::type> {
};

template <class C>
struct not_has_print_on<C,typename boost::enable_if<boost::is_pointer<C> >::type> {
};


template <class C>
struct not_has_print_on<boost::reference_wrapper<C> > : public not_has_print_on<C> {};

/*template<class A>
static const char * dbgstr(const A &a);

template<class A,class Writer>
static const char * dbgstr(const A &a,Writer writer);
*/

template <class A>
inline void dbgout(std::ostream &o,const A a,typename boost::enable_if<boost::is_pointer<A> >::type* dummy = 0) {
  if (a == NULL) {
    o << "NULL";
  } else {

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif
      o << "0x" << std::hex << (size_t)a << std::dec << "=&((";
#ifdef _MSC_VER
#pragma warning( pop )
#endif
  dbgout(o,*a);
  o << "))";
  }
}


// what follows relies on SFINAE (google) to identify whether an object supports print_on (with or without a Writer param)
template<class A>
inline void dbgout(std::ostream &o,const A &a, typename has_print_on_plain<A>::type* dummy = 0) {
  deref(a).print_on(o);
}

template<class A>
inline void dbgout(std::ostream &o,const A &a, typename has_print_on_writer<A>::type* dummy = 0) {
  deref(a).print_on(o,DebugWriter());
}

template<class A>
inline void dbgout(std::ostream &o,const A &a, typename not_has_print_on<A>::type* dummy = 0) {
    o << deref(a);
}


template<class A,class W>
inline void dbgout(std::ostream &o,const A &a,W w,typename has_print_on_plain<A>::type* dummy = 0) {
  deref(a).print_on(o);
}

template<class A,class W>
inline void dbgout(std::ostream &o,const A &a,W w,typename has_print_on_writer<A>::type* dummy = 0) {
  deref(a).print_on(o,w);
}

inline void dbgout(std::ostream &o,const char *a) {
  o << a;
}

/*
template <class A>
inline void dbgout(std::ostream &o,const A &a, typename boost::enable_if<boost::is_arithmetic<A> >::type* dummy = 0) {
  o << a;
}


inline void dbgout(std::ostream &o,const std::string &a) {
  o << a;
}
*/


#ifdef DEBUG
#include "threadlocal.hpp"

namespace DBP {
extern unsigned DBPdepth;
    struct DBPscopedepth {
        DBPscopedepth() { ++DBPdepth;}
        ~DBPscopedepth() { --DBPdepth;}
    };
#ifdef MAIN
    unsigned DBPdepth=0;
    bool DBPdisable=false;
#endif
};

#ifdef _MSC_VER
#include <windows.h>
#define DBPSS(a) do { (OutputDebugString((const char *)((a).c_str()))); Config::debug() << (a); } while(0)
#define DBPS(a) do { (OutputDebugString((const char *)(a))); Config::debug() << (a); } while(0)

#else
#define DBPSS(a) (Config::debug() << (a))
#define DBPS(a) (Config::debug() << (a))
#endif

#include <boost/preprocessor/stringize.hpp>
#if 0
#ifdef _MSV_VER
#undef BOOST_PP_STRINGIZE
#define STRINGIZE( L ) #L
#define MAKESTRING( M, L ) M(L)
#define BOOST_PP_STRINGIZE(L) MAKESTRING(STRINGIZE, L )
#endif
#endif
#define LINESTR dbgstr(__LINE__)
//BOOST_PP_STRINGIZE(__LINE__)

#define DBPRE for(unsigned DBPdepth_i=0;DBPdepth_i<DBP::DBPdepth;++DBPdepth_i) DBPS(" "); DBPS(__FILE__ ":");DBPSS(LINESTR);DBPS(":")

#define DBPIN ++DBP::DBPdepth
#define DBPOUT if (!DBP::DBPdepth) DBPC("warning: depth decreased below 0 with DBPOUT"); else --DBP::DBPdepth
#define DBPSCOPE DBP::DBPscopedepth DBPscopedepth

#define DBPENABLE(x) SetLocal<bool> DBPenablescope(DBP::DBPdisable,!x)
#define DBPOFF DBPENABLE(false)
#define DBPON DBPENABLE(true)

#define DBPOST DBPS("\n")
#define BDBP(a) do { if (!DBP::DBPdisable) { DBPS(" " #a "=_<");DBPSS(dbgstr(a));DBPS(">_");  }} while(0)

#define DBP(a) do { if (!DBP::DBPdisable) { DBPRE; BDBP(a);DBPOST;  }} while(0)
#define DBP2(a,b) do { if (!DBP::DBPdisable) { DBPRE; BDBP(a); BDBP(b);DBPOST;  }} while(0)
#define DBP3(a,b,c) do { if (!DBP::DBPdisable) { DBPRE; BDBP(a); BDBP(b); BDBP(c);DBPOST; }} while(0)
#define DBP4(a,b,c,d) do { if (!DBP::DBPdisable) { DBPRE; BDBP(a); BDBP(b); BDBP(c); BDBP(d); DBPOST; }} while(0)
#define DBP5(a,b,c,d,e) do { if (!DBP::DBPdisable) { DBPRE; BDBP(a); BDBP(b); BDBP(c); BDBP(d); BDBP(e); DBPOST; }} while(0)

//#define DBP2(a,p) do { if (!DBP::DBPdisable) { DBPS(DBPRE(a,__FILE__,__LINE__)  #a " = ");DBPSS(dbgstr(a,p));   }} while(0)
//#define DBP(a) do { if (!DBP::DBPdisable) { DBPS(DBPRE(a,__FILE__,__LINE__)  #a " = ");DBPSS(dbgstr(a));  }} while(0)
#define BDBPW(a,w) do { if (!DBP::DBPdisable) { DBPS(" " #a "=_<");DBPSS(dbgstr(a,w));DBPS(">_");  }} while(0)
#define DBPW(a,w) do { if (!DBP::DBPdisable) { DBPRE; BDBPW(a,w) ;DBPOST;  }} while(0)

#define DBPC(msg) do { if (!DBP::DBPdisable) { DBPRE; DBPS(" (" msg ")"); DBPOST;  }} while(0)
#define DBPC2(msg,a) do { if (!DBP::DBPdisable) { DBPRE; DBPS(" (" msg ")"); BDBP(a); DBPOST;  }} while(0)
#define DBPC3(msg,a,b) do { if (!DBP::DBPdisable) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); DBPOST;  }} while(0)
#define DBPC4(msg,a,b,c) do { if (!DBP::DBPdisable) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBP(c); DBPOST;  }} while(0)
#define DBPC5(msg,a,b,c,d) do { if (!DBP::DBPdisable) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBP(c); BDBP(d); DBPOST;  }} while(0)
#define DBPC4W(msg,a,b,c,w) do { if (!DBP::DBPdisable) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBPW(c,w); DBPOST;  }} while(0)



#else
#define DBPIN
#define DBPOUT
#define DBPSCOPE

#define DBP(a)
#define DBP2(a,b)
#define DBP3(a,b,c)
#define DBP4(a,b,c,d)
#define DBP5(a,b,c,d,e)

#define DBPC(msg)
#define DBPC2(msg,a)
#define DBPC3(msg,a,b)
#define DBPC4(msg,a,b,d)
#define DBPC5(msg,a,b,c,d)
#define DBPC4W(msg,a,b,c,w)

#define BDBPW(a,w)
#define DBPW(a,w)

#define DBPENABLE(x)
#define DBPOFF
#define DBPON

#endif

#if 0
static const std::string constEmptyString;
extern THREADLOCAL std::ostringstream dbgbuf;
extern THREADLOCAL std::string dbgstring;

#ifdef MAIN
THREADLOCAL std::ostringstream dbgbuf;
THREADLOCAL std::string dbgstring;
#endif
#endif

#define constEmptyString ""
template<class A>
std::string dbgstr(const A &a) {
//    std::ostringstream dbgbuf;
  //dbgbuf.str(constEmptyString);
  //dbgbuf.clear(); // error flags
  std::ostringstream dbgbuf;
  dbgout(dbgbuf,a);
  Assert(dbgbuf.good());
  return dbgbuf.str();
  //return dbgstring.c_str(); //FIXME: since string is a copy by std, may need to use a static string as well
}

template<class A,class W>
std::string dbgstr(const A &a,W w) {
      std::ostringstream dbgbuf;
  //dbgbuf.str(constEmptyString);
  //dbgbuf.clear(); // error flags
  dbgout(dbgbuf,a,w);
  Assert(dbgbuf.good());
  return dbgbuf.str();
  //return dbgstring.c_str(); //FIXME: since string is a copy by std, may need to use a static string as well
}

#undef constEmptyString

template<class A>
const char * dbgstrw(const A &a) {
  return dbgstr(a);
}

#endif
