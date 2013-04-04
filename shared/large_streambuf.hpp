#ifndef GRAEHL_SHARED__LARGE_STREAMBUF_HPP
#define GRAEHL_SHARED__LARGE_STREAMBUF_HPP

#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <fstream>
#include <new>

namespace graehl {

// must have at least the lifetime of the stream you attach this to (or else call detach(stream) before this dies)
template <std::size_t bufsize = 256*1024>
struct large_streambuf
{
  BOOST_STATIC_CONSTANT(std::size_t, size = bufsize);
  char buf[bufsize];
  large_streambuf() {}
  template <class S>
  large_streambuf(S &s)
  {
    attach_to_stream(s);
  }
  template <class S>
  void attach_to_stream(S &stream)
  {
    if (size)
      stream.rdbuf()->pubsetbuf(buf, size);
  }
  template <class S>
  void detach(S &stream) {
    stream.rdbuf()->pubsetbuf(0, 0);
  }



};


/**
   while this object exists, a basic_ostream or basic_istream will have a large buffer.

   for fstream, you must expand the streambuf *before* opening the file (this
   may be a linux bug), or else the buffer won't be used.

   either this object must last longer than the stream, or call
   resets(Stream *s) so you'll be able to continue using the stream later
*/

struct bigger_streambuf : boost::noncopyable
{
  std::size_t size;
  void *buf;
  std::streambuf *resetbuf;

  bigger_streambuf(std::size_t size) : size(size), buf(size?::operator new(size):0), resetbuf()
  {}

  template <class S>
  bigger_streambuf(std::size_t size, S &s, bool stream_outlives_buffer = true)
      : size(size), buf(size?::operator new(size):0)
  {
    attach_to_stream(s, stream_outlives_buffer);
  }
  template <class S>
  void attach_to_stream(S &s, bool stream_outlives_buffer = true)
  {
    if (size) {
      resetbuf = stream_outlives_buffer ? s.rdbuf() : 0;
      s.rdbuf()->pubsetbuf((char*)buf, size);
    }
  }
  void reset() {
    if (size) {
      if (resetbuf)
        resetbuf->pubsetbuf(0, 0); // this should flush if old buffer wasn't flushed yet
      ::operator delete(buf);
      size = 0;
    }
  }
  ~bigger_streambuf()
  {
    reset();
  }
};

}

#endif
