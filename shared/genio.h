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
#endif