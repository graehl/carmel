#ifndef GRAEHL__SHARED__NULL_OSTREAM_HPP
#define GRAEHL__SHARED__NULL_OSTREAM_HPP

//FIXME: doesn't seem to work w/ boost 1.37

#include <streambuf>
#include <ostream>
#include <string>
#include <boost/iostreams/stream.hpp>

namespace graehl {

template <class C>
struct null_device {
	typedef boost::iostreams::sink_tag category;
	typedef C char_type;
	std::streamsize write(const C*, std::streamsize sz)
	{
		return sz;
	}
};


template <class C>
class basic_null_ostream : public boost::iostreams::stream< null_device<C> > {};

typedef basic_null_ostream<char> null_ostream;

/* Jonathan:
   
I agree with the comment.  Specifically, the problem I'm imagining is that the parents (basic_ios, basic_ostream) of basic_null_ostream are being given the address of a as-yet-unconstructed basic_null_streambuf.

If those constructors actually use members of nullbuf in any way, other than just storing the pointer for future reference, that would be bad ;)  It sounds like STLPort does this.  Note that you can build an a regular ostream using a basic_null_streambuf:

basic_null_streambuf<char> b;
ostream o(&b);

but the convenience class, basic_null_ostream, is improperly designed (it can't use inheritance, i guess).

*/

/*
 note: this way of constructing a base class from a member is bad.  and it
 causes the stlport implementation of basic_ostream to blow up.
 rather than fix, i just provided a boost::iostreams implementation of
 null-stream.  it kind of reads better anyway... --michael
template <class C, class T = std::char_traits<C> >
class basic_null_streambuf : public std::basic_streambuf<C,T>
{
    virtual int overflow(typename T::int_type  c) { return T::not_eof(c); }
public:
    basic_null_streambuf() {}
};

typedef basic_null_streambuf<char> null_streambuf;


template <class C, class T = std::char_traits<C> >
class basic_null_ostream: public std::basic_ostream<C,T>
{
    basic_null_streambuf<C,T> nullbuf;
 public:
    basic_null_ostream() :
        std::basic_ios<C,T>(&nullbuf),
        std::basic_ostream<C,T>(&nullbuf)
    { init(&nullbuf); }
};

//typedef basic_null_ostream<char> null_ostream;
*/
}//graehl

#endif
