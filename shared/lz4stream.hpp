#ifndef GRAEHL__SHARED__LZ4STREAM_H
#define GRAEHL__SHARED__LZ4STREAM_H

#include <iostream>
#include <fstream>
#include <graehl/shared/lz4.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

namespace graehl {

struct lz4_filter
{

};

// see also file_descriptor_sink, mapped_file_sink
using boost::iostream::file_sink;

using boost::iostream::filtering_istream;
using boost::iostream::filtering_ostream;

template <class Impl>
struct ilz4stream : filtering_istream
{
public:
  void open(std::string const& name,int open_mode=std::ios::in)
  {
    assert(!opened);
    opened=true;
    push(file_sink(name,open_mode|std::ios::binary));
  }

  void close()
  {
    if (!opened) return;
    opened=false;
    pop();
    pop();
  }
};

template <class Impl>
struct olz4stream : filtering_istream
{
public:
  void open(std::string const& name,int open_mode=std::ios::in)
  {
    assert(!opened);
    opened=true;
    push(file_sink(name,open_mode|std::ios::binary));
  }

  void close()
  {
    if (!opened) return;
    opened=false;
    pop();
    pop();
  }
};

#if 0
// ----------------------------------------------------------------------------
// Internal classes to implement lz4stream. See below for user classes.
// ----------------------------------------------------------------------------

class lz4streambuf : public std::streambuf {
private:
  static const int bufferSize = 47+(1024*256);    // size of data buff
  // 47 totals 256 bytes under g++ for ilz4stream at the end.

  gzFile file;               // file handle for compressed file
  char buffer[bufferSize]; // data buffer
  bool opened;             // open/close state of stream
  int mode;               // I/O mode

  int flush_buffer();
  void handle_gzerror(); // throws exception
public:
#if defined(_WIN32) && !defined(CYGWIN) && !defined(EOF)
	enum {
		EOF = -1
	};
#endif

  lz4streambuf() : opened(0) {
    setp( buffer, buffer + (bufferSize-1));
    setg( buffer + 4,     // beginning of putback area
          buffer + 4,     // read position
          buffer + 4);    // end position
    // ASSERT: both input & output capabilities will not be used together
  }
  virtual bool is_open() { return opened; }
  lz4streambuf* open( const char* name, int open_mode);
  lz4streambuf* close();
  virtual ~lz4streambuf() { close(); } /* in a derived class, if your base class has a virtual destructor, your own destructor is automatically virtual. You might need an explicit destructor for other reasons, but there's no need to redeclare a destructor simply to make sure it is virtual. No matter whether you declare it with the virtual keyword, declare it without the virtual keyword, or don't declare it at all, it's still virtual. */

  virtual int overflow( int c = EOF);
  virtual int underflow();
  virtual int sync();
};

class lz4streambase : virtual public std::ios { // because istream, ostream also have virtual public ios base. want it shared.
protected:
  lz4streambuf buf;
public:
  lz4streambase() { init(&buf); }
  lz4streambase( const char* name, int open_mode);
  ~lz4streambase();
  void open( const char* name, int open_mode);
  void close();
  lz4streambuf* rdbuf() { return &buf; }
};

// ----------------------------------------------------------------------------
// User classes. Use ilz4stream and olz4stream analogously to ifstream and
// ofstream respectively. They read and write files based on the gz*
// function interface of the zlib. Files are compatible with gzip compression.
// ----------------------------------------------------------------------------

class ilz4stream : public lz4streambase, public std::istream {
public:
  ilz4stream() : std::istream(&buf) {}
  ilz4stream( const char* name, int open_mode = std::ios::in)
    : lz4streambase( name, std::ios::in | open_mode), std::istream( &buf) {}
  lz4streambuf* rdbuf() { return lz4streambase::rdbuf(); }
  void open( const char* name, int open_mode = std::ios::in) {
    lz4streambase::open( name, open_mode);
  }
};

class olz4stream : public lz4streambase, public std::ostream {
public:
  olz4stream() : std::ostream( &buf) {}
  olz4stream( const char* name, int mode =  std::ios::out)
    : lz4streambase( name, std::ios::out | mode), std::ostream( &buf) {}
  lz4streambuf* rdbuf() { return lz4streambase::rdbuf(); }
  void open( const char* name, int open_mode = std::ios::out) {
    lz4streambase::open( name, open_mode);
  }
};

#endif
} // namespace graehl

#endif
