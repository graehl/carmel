//Operators << and >> are generated from members read(in) and print(out).  Also, convenience macros for reading/validating
#ifndef GENIO_H
#define GENIO_H

#define GENIOGOOD std::ios_base::goodbit
#define GENIOBAD std::ios_base::failbit

// CHANGE: void print(ostream &) is fine.  we assume output never fails
/*
  Dietmar says:
  It is faster to use the stream buffer directly, ie. in the form of
'std::streambuf::sgetc()'. An equivalent use which I prefer over use of
'std::streambuf::sgetc()' is the use of
'std::istreambuf_iterator<char>' which gives an iterator interface to
stream buffers.
*/

// TODO: also: check out ipfx and opfx (std proposal, in libstdc++, replacement for sentry stuff)

#include <graehl/shared/debugprint.hpp>

// important: if you want to allow the first EOF/fail in your read routine to be
// fine (not an exception) don't use EXPECT... etc. or use EXPECT...FIRST

// very important: make your read exception safe (use funcs.hpp self_destruct or other guards/automatically managed resources)

// your class Arg must will provide Arg::read(is) ( is >> arg ) and Arg::print_to(os) (os << arg),
// returning std::ios_base::iostate (0,badbit,failbit ...)
// usage:
/*
  template <class charT, class Traits>
  std::basic_istream<class charT, class Traits>&
  operator >>
  (std::basic_istream<class charT, class Traits>& is, Arg &arg)
  {
  return gen_extractor(is,arg);
  }
*/
// this is necessary because of incomplete support for partial explicit template instantiation
// a more complete version could catch exceptions and set ios error

//PASTE THIS INTO CLASS
/*

  template <class charT, class Traits>
  std::ios_base::iostate
  print(std::basic_ostream<charT,Traits>& o) const
  {
    return GENIOGOOD;
  }

  template <class charT, class Traits>
  std::ios_base::iostate
  read(std::basic_istream<charT,Traits>& in)
  {
    return GENIOGOOD;
  fail:
    return GENIOBAD;

  }

*/


//PASTE THIS OUTSIDE CLASS C
/*
CREATE_INSERTER(C)
CREATE_EXTRACTOR(C)
CREATE_INSERTER_T1(C) // for classes with 1 template arg.
*/

#include <iostream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <stdexcept>

#define GEN_IO_(s,io,sentrytype) do {                                             \
        typename sentrytype,Traits>::sentry sentry(s,false);                           \
        if (sentry) {                                                 \
            std::ios_base::iostate err = io; \
            if (err==GENIOGOOD) s.clear();        \
            else if (err) s.setstate(err); \
        } } while(0)

// uses (template) charT, Traits
// s must be an (i)(o)stream reference; io returns GENIOGOOD or GENIOBAD (read or print)
#define GEN_EXTRACTOR(s,io) GEN_IO_(s,io,std::basic_istream<charT)
#define GEN_INSERTER(s,io)  do {                                             \
        typename std::basic_ostream<charT,Traits>::sentry sentry(s);                        \
        if (sentry) {                                                 \
            io; \
        } else throw std::runtime_error("Tried to print to invalid stream"); \
    } while(0)

//GEN_IO_(s,io,std::basic_ostream<charT)
// only difference is ostream sentry not istream sentry

#define GENIO_FAIL(s) do { s.setstate(GENIOBAD); }while(0)

template <class charT, class Traits, class Arg>
std::basic_istream<charT, Traits>&
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg)
{
  GEN_EXTRACTOR(s,arg.read(s));
  return s;
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_istream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.read(s);
    if (err)
        s.setstate(err);
    return s;
    */
}

// exact same as above but with o instead of i
template <class charT, class Traits, class Arg>
std::basic_ostream<charT, Traits>&
gen_inserter
    (std::basic_ostream<charT, Traits>& s, const Arg &arg)
{
    GEN_INSERTER(s,arg.print(s));
    return s;

/*  if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print(s);
    if (err)
        s.setstate(err);
    return s;*/
}

#define FROM_ISTREAM_READ_GEN                                                 \
    template <class Char,class Traits> \
    inline friend std::basic_istream<Char,Traits>& operator >>(std::basic_istream<Char,Traits> &i,self_type & me)     \
    { return gen_extractor(i,me); }


template <class charT, class Traits, class Arg, class Reader>
std::basic_istream<charT, Traits>&
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg, Reader read)
{
  GEN_EXTRACTOR(s,arg.read(s,read));
  return s;
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_istream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.read(s,read);
    if (err)
        s.setstate(err);
    return s;
    */
}

template <class charT, class Traits, class Arg, class Reader,class Flag>
std::basic_istream<charT, Traits>&
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg, Reader read, Flag f)
{
  GEN_EXTRACTOR(s,arg.read(s,read,f));
  return s;
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_istream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.read(s,read,f);
    if (err)
        s.setstate(err);
    return s;
    */
}


// exact same as above but with o instead of i
template <class charT, class Traits, class Arg, class R>
std::basic_ostream<charT, Traits>&
gen_inserter
    (std::basic_ostream<charT, Traits>& s, const Arg &arg, R r)
{
  GEN_INSERTER(s,arg.print(s,r));
  return s;
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print(s,r);
    if (err)
        s.setstate(err);
    return s;*/
}

// exact same as above but with o instead of i
template <class charT, class Traits, class Arg, class Q,class R>
std::basic_ostream<charT, Traits>&
gen_inserter
    (std::basic_ostream<charT, Traits>& s, const Arg &arg, Q q,R r)
{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4800 )
#endif

  GEN_INSERTER(s,arg.print(s,q,r));
  return s;
#ifdef _MSC_VER
#pragma warning( pop )
#endif
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print(s,q,r);
    if (err)
        s.setstate(err);
    return s;
    */
}


#define FRIEND_EXTRACTOR(C) \
template <class charT, class Traits> \
friend  std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg);


#define DEFINE_EXTRACTOR(C) \
  template <class charT, class Traits> \
std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg);

#define CREATE_EXTRACTOR(C) \
  template <class charT, class Traits> \
inline std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg) { \
    return gen_extractor(is,arg); }

#define CREATE_EXTRACTOR_T1(C) \
    template <class charT, class Traits,class T1>            \
inline std::basic_istream<charT,Traits>& operator >> \
    (std::basic_istream<charT,Traits>& is, C<T1> &arg) {        \
    return gen_extractor(is,arg); }

#define CREATE_EXTRACTOR_READER(C,R) \
  template <class charT, class Traits> \
inline std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg) { \
    return gen_extractor(is,arg,R); }

#define FRIEND_INSERTER(C) \
template <class charT, class Traits> \
friend std::basic_ostream<charT,Traits>& operator << \
 (std::basic_ostream<charT,Traits>& os, const C &arg);


#define DEFINE_INSERTER(C) \
  template <class charT, class Traits> \
std::basic_ostream<charT,Traits>& operator << \
 (std::basic_ostream<charT,Traits>& os, const C &arg);

#define CREATE_INSERTER(C) \
  template <class charT, class Traits> \
inline std::basic_ostream<charT,Traits>& operator << \
 (std::basic_ostream<charT,Traits>& os, const C &arg) { \
    return gen_inserter(os,arg); }

#define CREATE_INSERTER_T1(C)          \
    template <class charT, class Traits,class T1>            \
inline std::basic_ostream<charT,Traits>& operator << \
                                                     (std::basic_ostream<charT,Traits>& os, const C<T1> &arg) {  \
    return gen_inserter(os,arg); }


#define GENIOSETBAD(in) do { in.setstate(GENIOBAD); } while(0)

/*
#define GENIO_CHECK(inop) do { ; if (!(inop).good()) return GENIOBAD; } while(0)
#define GENIO_CHECK_ELSE(inop,fail) do {  if (!(inop).good()) { fail; return GENIOBAD; } } while(0)
*/

#define GENIO_read   template <class charT, class Traits> \
  std::ios_base::iostate \
  read(std::basic_istream<charT,Traits>& in)

#define GENIO_print     typedef void has_print; \
 template <class charT, class Traits> \
  std::ios_base::iostate \
  print(std::basic_ostream<charT,Traits>& o) const

#define GENIO_print_writer     typedef void has_print_writer; \
 template <class charT, class Traits, class Writer> \
  std::ios_base::iostate \
  print(std::basic_ostream<charT,Traits>& o,Writer w) const

#define GENIO_read_any   template <class T> \
  std::ios_base::iostate \
    read(T& in)


/*
template <class charT, class Traits>
  std::ios_base::iostate
  print(std::basic_ostream<charT,Traits>& o=std::cerr) const
*/

#include <limits.h>

/*
template <class charT, class Traits>
inline typename Traits::int_type getch_space_comment(std::basic_istream<charT,Traits>&in,charT comment_char='#',charT newline_char='\n') {
  charT c;
  for(;;) {
    if (!(in >> c)) return Traits::eof();
    if (c==comment_char)
      in.ignore(INT_MAX,newline_char);
    else
      return c;
  }
}
*/

template <class charT, class Traits>
inline std::basic_istream<charT,Traits>& skip_comment(std::basic_istream<charT,Traits>&in,charT comment_char='%',charT newline_char='\n') {
  charT c;
  for(;;) {
    if (!(in >> c).good()) break;
    if (c==comment_char) {
      if (!(in.ignore(INT_MAX,newline_char)).good())
        break;
    } else {
      in.unget();
      break;
    }
  }
  return in;
}



template <class charT, class Traits>
inline std::basic_istream<charT,Traits>& skip_chars(std::basic_istream<charT,Traits>&in,charT ch) {
    char c;
    while (in.get(c))
        if (c!=ch) {
            in.unget();
            break;
        }
    return in;
}




//#define GENIO_THROW2(a,b) DBPC2(a,b)
typedef std::ios_base::failure genio_exception;
#define GENIO_EOF_OK bool GENIO_eof_ok=true
#define GENIO_EOF_BAD GENIO_eof_ok=false
#define GENIO_THROW(a) do {  throw genio_exception(a); } while(0)
#define GENIO_THROW2(a,b) do { throw genio_exception(std::string(a).append(b)); } while(0)
#define BREAK_ONCH_SPACE(a) if (!(in>>c)) break;    \
                            if (c==a) break; \
                            in.unget()
#define EXPECTI_FIRST(inop) do {  if (!(inop).good())  goto fail; } while(0)
#define EXPECTI(inop) do {  if (!(inop).good()) { GENIO_THROW2("expected input failed: ",#inop); } } while(0)
//#define EXPECTI_COMMENT(inop) do { if (!(inop).good()) { goto fail; } } while(0)
#define I_COMMENT(inop) (skip_comment(in) && inop)
#define EXPECTI_COMMENT_FIRST(inop) do { if ( !(skip_comment(in).good()&&(inop).good()) ) { goto fail; } } while(0)
#define EXPECTI_COMMENT(inop) do { if (!(skip_comment(in).good()&&(inop).good())) { GENIO_THROW2("expected input failed: ",#inop); } } while(0)
#define EXPECTCH(a) do { if (!in.get(c).good()) { GENIO_THROW2("expected input unavailable: ",#a); } if (c != a) { GENIO_THROW2("expected input failed: ",#a); } } while(0)
#define EXPECTCH_SPACE(a) do { if (!(in>>c).good()) { GENIO_THROW2("expected input unavailable: ",#a); } if (c != a) { GENIO_THROW2("expected input failed: ",#a); } } while(0)
//#define EXPECTCH_SPACE_COMMENT(a) do { if (!(in>>c).good()) goto fail; if (c != a) goto fail; } while(0)
#define EXPECTCH_SPACE_COMMENT_FIRST(a) do { if (!(skip_comment(in) && (in>>c))) goto fail; if (c != a) goto fail; } while(0)
#define EXPECTCH_SPACE_COMMENT(a) do { if (!(skip_comment(in).good()&&(in>>c).good())) { GENIO_THROW2("expected input unavailable: ",#a); }if (c != a) { GENIO_THROW2("expected input failed: ",#a); } } while(0)
//#define PEEKCH(a,i,e) do { if (!in.get(c).good()) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)
//#define PEEKCH_SPACE(a,i,e) do { if (!(in>>c).good()) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)
//#define IFCH_SPACE_COMMENT(a) if (!(skip_comment(in).good()&&(in>>c).good())) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)
//#define SKIP_CHARS(i,ch) do { while (in.get(c)) { if (c!=ch) { in.unget();break; } } } while(0)
#define GENIORET std::ios_base::iostate


#endif
