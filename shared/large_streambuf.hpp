#ifndef GRAEHL_SHARED__LARGE_STREAMBUF_HPP
#define GRAEHL_SHARED__LARGE_STREAMBUF_HPP

#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <fstream>
#include <new>

namespace graehl {

template <std::size_t bufsize=256*1024>
struct large_streambuf
{
    BOOST_STATIC_CONSTANT(std::size_t,size=bufsize);
    char buf[bufsize];
    large_streambuf() {}
    template <class S>
    large_streambuf(S &s)
    {
        attach_to_stream(s);
    }
    template <class S>
    void attach_to_stream(S &s)
    {
        if (size)
            s.rdbuf()->pubsetbuf(buf,size);
    }

};

// must have at least the lifetime of the stream you construct this with.  for fstream, you must expand the streambuf *before* opening the file, or the pubsetbuf call has no effect.
struct bigger_streambuf : boost::noncopyable
{
    std::size_t size;
    void *buf;

    bigger_streambuf(std::size_t size) : size(size),buf(size?::operator new(size):0)
    {}

    template <class S>
    bigger_streambuf(std::size_t size,S &s) : size(size),buf(size?::operator new(size):0)
    {
        attach_to_stream(s);
    }
    template <class S>
    void attach_to_stream(S &s)
    {
        if (size)
            s.rdbuf()->pubsetbuf((char*)buf,size);
    }
    ~bigger_streambuf()
    {
        if (size)
            ::operator delete(buf);
    }
};



}


#endif
