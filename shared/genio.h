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
	return std::ios_base::goodbit;
  }

  template <class charT, class Traits>
  std::ios_base::iostate 
  get_from(std::basic_istream<charT,Traits>& in)
  {
	return std::ios_base::goodbit;
  fail:
	return std::ios_base::badbit;
	
  }

*/


//PASTE THIS OUTSIDE CLASS C
/*
CREATE_INSERTER(C)
CREATE_EXTRACTOR(C)
*/

template <class charT, class Traits, class Arg>
std::basic_istream<charT, Traits>& 
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg)
{
	if (!s.good()) return s;
	std::ios_base::iostate err = std::ios_base::goodbit;
	typename std::basic_istream<charT, Traits>::sentry sentry(s);
	if (sentry)
		err = arg.get_from(s);
	if (err)
		s.setstate(err);
	return s;
}

// exact same as above but with o instead of i
template <class charT, class Traits, class Arg>
std::basic_ostream<charT, Traits>& 
gen_inserter
	(std::basic_ostream<charT, Traits>& s, const Arg &arg)
{
	if (!s.good()) return s;
	std::ios_base::iostate err = std::ios_base::goodbit;
	typename std::basic_ostream<charT, Traits>::sentry sentry(s);
	if (sentry)
		err = arg.print_on(s);
	if (err)
		s.setstate(err);
	return s;
}


template <class charT, class Traits, class Arg, class Reader>
std::basic_istream<charT, Traits>& 
gen_extractor
(std::basic_istream<charT, Traits>& s, Arg &arg, Reader &read)
{
	if (!s.good()) return s;
	std::ios_base::iostate err = std::ios_base::goodbit;
	typename std::basic_istream<charT, Traits>::sentry sentry(s);
	if (sentry)
		err = arg.get_from(s,read);
	if (err)
		s.setstate(err);
	return s;
}

// exact same as above but with o instead of i
template <class charT, class Traits, class Arg, class R>
std::basic_ostream<charT, Traits>& 
gen_inserter
	(std::basic_ostream<charT, Traits>& s, const Arg &arg, const R &r)
{
	if (!s.good()) return s;
	std::ios_base::iostate err = std::ios_base::goodbit;
	typename std::basic_ostream<charT, Traits>::sentry sentry(s);
	if (sentry)
		err = arg.print_on(s,r);
	if (err)
		s.setstate(err);
	return s;
}

// exact same as above but with o instead of i
template <class charT, class Traits, class Arg, class Q,class R>
std::basic_ostream<charT, Traits>& 
gen_inserter
	(std::basic_ostream<charT, Traits>& s, const Arg &arg, const Q &q,const R &r)
{
	if (!s.good()) return s;
	std::ios_base::iostate err = std::ios_base::goodbit;
	typename std::basic_ostream<charT, Traits>::sentry sentry(s);
	if (sentry)
		err = arg.print_on(s,q,r);
	if (err)
		s.setstate(err);
	return s;
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

#define GENIO_CHECK(inop) do { ; if (!(inop).good()) return std::ios_base::badbit; } while(0)
#define GENIO_CHECK_ELSE(inop,fail) do {  if (!(inop).good()) { fail; return std::ios_base::badbit; } } while(0)

#define GENIO_get_from   template <class charT, class Traits> \
  std::ios_base::iostate \
  get_from(std::basic_istream<charT,Traits>& in)

#define GENIO_print_on     template <class charT, class Traits> \
  std::ios_base::iostate \
  print_on(std::basic_ostream<charT,Traits>& o) const

#define EXPECTI(inop) do { ; if (!(inop).good()) goto fail; } while(0)
#define EXPECTCH(a) do { if (!in.get(c).good()) goto fail; if (c != a) goto fail; } while(0)
#define EXPECTCH_SPACE(a) do { if (!(in>>c).good()) goto fail; if (c != a) goto fail; } while(0)
#define PEEKCH(a,i,e) do { if (!in.get(c).good()) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)
#define PEEKCH_SPACE(a,i,e) do { if (!(in>>c).good()) goto fail; if (c==a) { i } else { in.unget(); e } } while(0)

#endif
