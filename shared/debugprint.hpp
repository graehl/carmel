// dbgout(out,var) function with default implementation; DB* convenience macros.  nested debug dumps.
#ifndef GRAEHL__SHARED__DEBUGPRINT_HPP
#define GRAEHL__SHARED__DEBUGPRINT_HPP

#ifdef GRAEHL__SINGLE_MAIN
#define GRAEHL__DEBUG_PRINT_MAIN
#endif

#ifdef DEBUG
# ifndef GRAEHL__DEBUG_PRINT
# define GRAEHL__DEBUG_PRINT
#endif
#endif

/* SUFFIXES:
   W means with writer; DBPW(a,w) goes to a.print(dbgout,w)
 */

/// In your clase, define a type: if you defined a
///  print(ostream &) const
///   method like genio.h: GENIO_print
///  typedef void has_print;
/// or if has two arg print(ostream &, Writer &w)
///   typedef void has_print_writer;
/// if you inherit from a class that has defined one of these, override it with some other type than void: typedef bool has_print_writer would disable

#include <graehl/shared/threadlocal.hpp>
#include <graehl/shared/myassert.h>
#include <sstream>
#include <graehl/shared/byref.hpp>
#include <iostream>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <graehl/shared/has_print.hpp>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/threadlocal.hpp>

#ifdef DEBUG
#define DEBUG_SEGFAULT Assert(0)
#else
#define DEBUG_SEGFAULT
#endif

namespace graehl {


/*template<class A>
static const char * dbgstr(const A &a);

template<class A,class Writer>
static const char * dbgstr(const A &a,Writer writer);
*/

inline void dbgout(std::ostream &o,const void *a) {
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif
    o << "0x" << std::hex << (size_t)a << std::dec;;
#ifdef _MSC_VER
#pragma warning( pop )
#endif
}

inline void dbgout(std::ostream &o,void *a) {
    dbgout(o,(const void *)a);
}


// what follows relies on SFINAE (google) to identify whether an object supports print (with or without a Writer param)
template<class A>
inline void dbgout(std::ostream &o,const A &a, typename has_print_plain<A>::type* dummy = 0) {
    (void)dummy;
    deref(a).print(o);
}

template<class A>
inline void dbgout(std::ostream &o,const A &a, typename not_has_print<A>::type* dummy = 0) {
    (void)dummy;
    o << deref(a);
}


template<class A,class W>
inline void dbgout(std::ostream &o,const A &a,W w,typename has_print_plain<A>::type* dummy = 0) {
    (void)dummy;
    deref(a).print(o);
}

template<class A,class W>
inline void dbgout(std::ostream &o,const A &a,W w,typename has_print_writer<A>::type* dummy = 0) {
    (void)dummy;
    deref(a).print(o,w);
}

inline void dbgout(std::ostream &o,const char *a) {
  o << a;
}

/*
#define HEX_THRESHOLD 100
template <class A>
inline void dbgout(std::ostream &o,const A &a, typename boost::enable_if<boost::is_arithmetic<A> >::type* dummy = 0) {
    if (a > HEX_THRESHOLD)
        o << a << "(=0x" << std::hex << (size_t)a << std::dec << ")";
;
}

*/

template <class A>
inline void dbgout_hex(std::ostream &o,const A &a) {
    o << a << "(=0x" << std::hex << (size_t)a << std::dec << ")";
}

inline void dbgout(std::ostream &o,unsigned long a) {
    dbgout_hex(o,a);
}

inline void dbgout(std::ostream &o,unsigned a) {
    dbgout_hex(o,a);
}

inline void dbgout(std::ostream &o,unsigned char a) {
    dbgout_hex(o,a);
}

inline void dbgout(std::ostream &o,unsigned short a) {
    dbgout_hex(o,a);
}



template <class A>
inline void dbgout(std::ostream &o,const A a,typename boost::enable_if<boost::is_pointer<A> >::type* dummy = 0) {
  (void)dummy;
  if (a == NULL) {
    o << "NULL";
  } else {
      dbgout(o,(void *)a);
      o << "=&((";
      dbgout(o,*a);
      o << "))";
  }
}

struct DebugWriter
{
  template <class charT, class Traits,class Label>
  std::basic_ostream<charT,Traits>&
  operator()(std::basic_ostream<charT,Traits>& o,const Label &l) const {
    dbgout(o,l);
    return o;
  }
};

template<class A>
inline void dbgout(std::ostream &o,const A &a, typename has_print_writer<A>::type* dummy = 0) {
    (void)dummy;
    deref(a).print(o,DebugWriter());
}


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
}//graehl

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef VC_EXTRALEAN
# define VC_EXTRALEAN 1
#endif
#ifndef NOMINMAX
# define NOMINMAX 1
#endif
#include <windows.h>
#undef min
#undef max
#undef DELETE

#define DBPSS(a) do { (OutputDebugString((LPCTSTR)((a).c_str()))); if (DBP::logstream) *DBP::logstream << (a); } while(0)
#define DBPS(a) do { (OutputDebugString((LPCTSTR)(a))); if (DBP::logstream) *DBP::logstream << (a); } while(0)

#else
#define DBPSS(a) do { if (DBP::logstream) *DBP::logstream << (a); } while(0)
#define DBPS(a) do { if (DBP::logstream) *DBP::logstream << (a); } while(0)
#endif

#include <boost/preprocessor/stringize.hpp>
#if 0
# ifdef _MSV_VER
#  undef BOOST_PP_STRINGIZE
#  define STRINGIZE( L ) #L
#  define MAKESTRING( M, L ) M(L)
#  define BOOST_PP_STRINGIZE(L) MAKESTRING(STRINGIZE, L )
# endif
# endif
#define LINESTR graehl::dbgstr(__LINE__)
//BOOST_PP_STRINGIZE(__LINE__)

#define DBPRE DBP::print_indent();DBPS(__FILE__ ":");DBPSS(LINESTR);DBPS(":")
#define DBPOST DBPS("\n")

#define SDBPOST(o) o << std::endl

#define SDBPRE(o) DBP::print_indent(o);o << __FILE__ ":" << LINESTR<<":"
#define SBDBP(o,a) do { if (1) { DBPS(" " #a "=_<");o << graehl::dbgstr(a);DBPS(">_");  }} while(0)
#define BDBP(a) do { if (1) { DBPS(" " #a "=_<");DBPSS(graehl::dbgstr(a));DBPS(">_");  }} while(0)
#define BDBPW(a,w) do { if (1) { DBPS(" " #a "=_<");DBPSS(graehl::dbgstr(a,w));DBPS(">_");  }} while(0)

#define DBPW(a,w) do { if (DBPISON) { DBPRE; BDBPW(a,w) ;DBPOST;  }} while(0)

#ifdef GRAEHL__DEBUG_PRINT
#define DBP_IN ++DBP::depth
#define DBP_OUT if (!DBP::depth) DBPC("warning: depth decreased below 0 with DBPOUT"); else --DBP::DBPdepth
#define DBP_SCOPE DBP::scopedepth DBP9423scopedepth ## __LINE__

#define DBP_ENABLE(x) graehl::SetLocal<bool> DBPenablescope_line_## __LINE__(DBP::disable,!x)
#define DBP_OFF DBP_ENABLE(false)
#define DBP_ON DBP_ENABLE(true)

#define DBP_VERBOSE(x) SetLocal<int> DBPverbosescope_line_ ## __LINE__(DBP::current_chat,(x))
#define DBP_INC_VERBOSE graehl::SetLocal<int> DBPverbosescope_line_ ## __LINE__(DBP::current_chat,DBP::current_chat+1)
#define DBP_ADD_VERBOSE(x) graehl::SetLocal<int> DBPverbosescope_line_ ## __LINE__ (DBP::current_chat,DBP::current_chat+(x))


#define DBPISON DBP::is_enabled()


#define DBP(a) do { if (DBPISON) { DBPRE; BDBP(a);DBPOST;  }} while(0)
#define DBP2(a,b) do { if (DBPISON) { DBPRE; BDBP(a); BDBP(b);DBPOST;  }} while(0)
#define DBP3(a,b,c) do { if (DBPISON) { DBPRE; BDBP(a); BDBP(b); BDBP(c);DBPOST; }} while(0)
#define DBP4(a,b,c,d) do { if (DBPISON) { DBPRE; BDBP(a); BDBP(b); BDBP(c); BDBP(d); DBPOST; }} while(0)
#define DBP5(a,b,c,d,e) do { if (DBPISON) { DBPRE; BDBP(a); BDBP(b); BDBP(c); BDBP(d); BDBP(e); DBPOST; }} while(0)

//#define DBP2(a,p) do { if (DBPISON) { DBPS(DBPRE(a,__FILE__,__LINE__)  #a " = ");DBPSS(graehl::dbgstr(a,p));   }} while(0)
//#define DBP(a) do { if (DBPISON) { DBPS(DBPRE(a,__FILE__,__LINE__)  #a " = ");DBPSS(graehl::dbgstr(a));  }} while(0)

#define DBPC(msg) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); DBPOST;  }} while(0)
#define DBPC2(msg,a) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); BDBP(a); DBPOST;  }} while(0)
#define DBPC3(msg,a,b) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); DBPOST;  }} while(0)
#define DBPC4(msg,a,b,c) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBP(c); DBPOST;  }} while(0)
#define DBPC5(msg,a,b,c,d) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBP(c); BDBP(d); DBPOST;  }} while(0)
#define DBPC6(msg,a,b,c,d,e) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBP(c); BDBP(d); BDBP(e); DBPOST;  }} while(0)
#define DBPC4W(msg,a,b,c,w) do { if (DBPISON) { DBPRE; DBPS(" (" msg ")"); BDBP(a); BDBP(b); BDBPW(c,w); DBPOST;  }} while(0)

#else
#define DBP_IN
#define DBP_OUT
#define DBP_SCOPE

#define DBP_ENABLE(x)
#define DBP_OFF
#define DBP_ON

#define DBP_VERBOSE(x)
#define DBP_INC_VERBOSE
#define DBP_ADD_VERBOSE(x)

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
#define DBPC6(msg,a,b,c,d,e)
#define DBPC4W(msg,a,b,c,w)

#define DBPENABLE(x)
#define DBPOFF
#define DBPON

#endif

#define RDBP(a) do { if (1) { DBPRE; BDBP(a);DBPOST;  }} while(0)
#define RDBP2(a,b) do { if (1) { DBPRE; BDBP(a); BDBP(b);DBPOST;  }} while(0)
#define RDBP3(a,b,c) do { if (1) { DBPRE; BDBP(a); BDBP(b); BDBP(c);DBPOST; }} while(0)
#define RDBP4(a,b,c,d) do { if (1) { DBPRE; BDBP(a); BDBP(b); BDBP(c); BDBP(d); DBPOST; }} while(0)
#define RDBP5(a,b,c,d,e) do { if (1) { DBPRE; BDBP(a); BDBP(b); BDBP(c); BDBP(d); BDBP(e); DBPOST; }} while(0)

#define SDBP(o,a) do { if (1) { SDBPRE(o); SBDBP(o,a);SDBPOST(o);  }} while(0)
#define SDBP2(o,a,b) do { if (1) { SDBPRE(o); SBDBP(o,a); SBDBP(o,b);SDBPOST(o);  }} while(0)
#define SDBP3(o,a,b,c) do { if (1) { SDBPRE(o); SBDBP(o,a); SBDBP(o,b); SBDBP(o,c);SDBPOST(o); }} while(0)
#define SDBP4(o,a,b,c,d) do { if (1) { SDBPRE(o); SBDBP(o,a); SBDBP(o,b); SBDBP(o,c); SBDBP(o,d); SDBPOST(o); }} while(0)
#define SDBP5(o,a,b,c,d,e) do { if (1) { SDBPRE(o); SBDBP(o,a); SBDBP(o,b); SBDBP(o,c); SBDBP(o,d); SBDBP(o,e); SDBPOST(o); }} while(0)

#if 0
static const std::string constEmptyString;
extern THREADLOCAL std::ostringstream dbgbuf;
extern THREADLOCAL std::string dbgstring;

#ifdef GRAEHL__SINGLE_MAIN
THREADLOCAL std::ostringstream dbgbuf;
THREADLOCAL std::string dbgstring;
#endif
#endif

namespace DBP {
    extern unsigned depth;
    extern bool disable;
    extern int current_chat;
    extern int chat_level;
    extern std::ostream *logstream;
    struct scopedepth {
        scopedepth() { ++depth;}
        ~scopedepth() { --depth;}
    };
    inline bool is_enabled() {
        return (!disable && current_chat <= chat_level && logstream);
    }
    void print_indent();
    void print_indent(std::ostream &o);
    void set_loglevel(int loglevel=0);
    void set_logstream(std::ostream &o=std::cerr);
#ifdef GRAEHL__DEBUG_PRINT_MAIN
    int current_chat;
    int chat_level;
    std::ostream *logstream=&std::cerr;
    void print_indent() {
        for(unsigned i=0;i<depth;++i)
            DBPS(' ');
    }
    void print_indent(std::ostream &o) {
        for(unsigned i=0;i<depth;++i)
            o << ' ';
    }
    void set_loglevel(int loglevel){
        chat_level=loglevel;
    }
    void set_logstream(std::ostream *o) {
        logstream=o;
    }

    unsigned depth=0;
    bool disable=false;
#endif
}//ns


#endif
