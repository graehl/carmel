#ifndef GENIO_H
#define GENIO_H 1


// your class Arg must will provide Arg::get_from(is) ( is >> arg ) and Arg::print_to(os) (os << arg),
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
  print_on(std::basic_ostream<charT,Traits>& o) const
  {
    return GENIOGOOD;
  }

  template <class charT, class Traits>
  std::ios_base::iostate
  get_from(std::basic_istream<charT,Traits>& in)
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
*/

#include <iostream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>

template <class C,class charT, class Traits>
inline void out_always_quote(std::basic_ostream<charT,Traits> &out, const C& data) {
    stringstream s;
    s << data;
    char c;
    while (s.get(c)) {
        if (c == '"')
            out.put('\\');
        out.put(c);
    }
}

template <class C,class charT, class Traits>
inline void out_quote(std::basic_ostream<charT,Traits> &out, const C& data) {
    typedef std::basic_string<charT,Traits> String;
    String s=boost::lexical_cast<String>(data);
    if (s.find('"') == String::npos)
        out << s;
    else {
        out << '"';
        for (typename String::iterator i=s.begin(),e=s.end();i!=e;++i) {
            char c=*i;
            if (c == '"')
                out.put('\\');
            out.put(c);
        }
        out << '"';
    }
}

#define ERROR_CONTEXT_CHARS 50
template <class Ic,class It,class Oc,class Ot>
inline void show_error_context(std::basic_istream<Ic,It>  &in,std::basic_ostream<Oc,Ot> &out) {
    char context[ERROR_CONTEXT_CHARS];
    in.clear();
    in.get(context,ERROR_CONTEXT_CHARS,'\n');
    out << "... " << context << std::endl << "   ^" << std::endl;
}

// uses (template) charT, Traits
// s must be an (i)(o)stream reference; io returns GENIOGOOD or GENIOBAD (get_from or print_on)
#define GEN_EXTRACTOR(s,io) do { \
    if (!s.good()) return s; \
    std::ios_base::iostate err = std::ios_base::goodbit; \
    typename std::basic_istream<charT, Traits>::sentry sentry(s); \
    if (sentry) { err = io; } \
    if (err) s.setstate(err); \
    return s; \
} while(0)

// only difference is ostream sentry not istream sentry
#define GEN_INSERTER(s,print_on) do { \
    if (!s.good()) return s; \
    std::ios_base::iostate err = std::ios_base::goodbit; \
    typename std::basic_ostream<charT, Traits>::sentry sentry(s); \
    if (sentry) { print_on; } \
    if (err) s.setstate(err); \
    return s; \
} while(0)

template <class charT, class Traits, class Arg>
std::basic_istream<charT, Traits>&
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg)
{
  GEN_EXTRACTOR(s,arg.get_from(s));
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_istream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.get_from(s);
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
    GEN_INSERTER(s,arg.print_on(s));

/*  if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print_on(s);
    if (err)
        s.setstate(err);
    return s;*/
}


template <class charT, class Traits, class Arg, class Reader>
std::basic_istream<charT, Traits>&
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg, Reader read)
{
  GEN_EXTRACTOR(s,arg.get_from(s,read));
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_istream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.get_from(s,read);
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
  GEN_EXTRACTOR(s,arg.get_from(s,read,f));
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_istream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.get_from(s,read,f);
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
  GEN_INSERTER(s,arg.print_on(s,r));
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print_on(s,r);
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

  GEN_INSERTER(s,arg.print_on(s,q,r));
#ifdef _MSC_VER
#pragma warning( pop )
#endif
  /*
    if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print_on(s,q,r);
    if (err)
        s.setstate(err);
    return s;
    */
}




#define DEFINE_EXTRACTOR(C) \
  template <class charT, class Traits> \
std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg);

#define CREATE_EXTRACTOR(C) \
  template <class charT, class Traits> \
inline std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg) { \
    return gen_extractor(is,arg); }

#define CREATE_EXTRACTOR_READER(C,R) \
  template <class charT, class Traits> \
inline std::basic_istream<charT,Traits>& operator >> \
 (std::basic_istream<charT,Traits>& is, C &arg) { \
    return gen_extractor(is,arg,R); }


#define DEFINE_INSERTER(C) \
  template <class charT, class Traits> \
std::basic_ostream<charT,Traits>& operator << \
 (std::basic_ostream<charT,Traits>& os, const C arg);

#define CREATE_INSERTER(C) \
  template <class charT, class Traits> \
inline std::basic_ostream<charT,Traits>& operator << \
 (std::basic_ostream<charT,Traits>& os, const C arg) { \
    return gen_inserter(os,arg); }

#define GENIOGOOD std::ios_base::goodbit
#define GENIOBAD std::ios_base::badbit

#define GENIOSETBAD(in) do { in.setstate(GENIOBAD); } while(0)

/*
#define GENIO_CHECK(inop) do { ; if (!(inop).good()) return GENIOBAD; } while(0)
#define GENIO_CHECK_ELSE(inop,fail) do {  if (!(inop).good()) { fail; return GENIOBAD; } } while(0)
*/

#define GENIO_get_from   template <class charT, class Traits> \
  std::ios_base::iostate \
  get_from(std::basic_istream<charT,Traits>& in)

#define GENIO_print_on     typedef void has_print_on; \
 template <class charT, class Traits> \
  std::ios_base::iostate \
  print_on(std::basic_ostream<charT,Traits>& o) const

#define GENIO_print_on_writer     typedef void has_print_on_writer; \
 template <class charT, class Traits, class Writer> \
  std::ios_base::iostate \
  print_on(std::basic_ostream<charT,Traits>& o,Writer w) const


/*
template <class charT, class Traits>
  std::ios_base::iostate
  print_on(std::basic_ostream<charT,Traits>& o=std::cerr) const
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


#define EXPECTI(inop) do { ; if (!(inop).good()) goto fail; } while(0)
//#define EXPECTI_COMMENT(inop) do { ; if (!(inop).good()) goto fail; } while(0)
#define EXPECTI_COMMENT(inop) do { ; if (!(skip_comment(in).good()&&(inop).good())) goto fail; } while(0)
#define EXPECTCH(a) do { if (!in.get(c).good()) goto fail; if (c != a) goto fail; } while(0)
#define EXPECTCH_SPACE(a) do { if (!(in>>c).good()) goto fail; if (c != a) goto fail; } while(0)
//#define EXPECTCH_SPACE_COMMENT(a) do { if (!(in>>c).good()) goto fail; if (c != a) goto fail; } while(0)
#define EXPECTCH_SPACE_COMMENT(a) do { if (!(skip_comment(in).good()&&(in>>c).good())) goto fail;if (c != a) goto fail; } while(0)
//#define PEEKCH(a,i,e) do { if (!in.get(c).good()) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)
//#define PEEKCH_SPACE(a,i,e) do { if (!(in>>c).good()) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)
//#define IFCH_SPACE_COMMENT(a) if (!(skip_comment(in).good()&&(in>>c).good())) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)

#define GENIORET std::ios_base::iostate
#endif
