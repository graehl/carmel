#ifndef CHARBUF_HPP
#define CHARBUF_HPP

// no separate implementation for now, just #define MAIN in one source file that includes this
#include "dynarray.h"
#ifndef CHARBUF_INIT_SIZE
#define CHARBUF_INIT_SIZE 4096
#endif


typedef DynamicArray<char> CharBuf;

// USAGE: clear() before you use, repeatedly push_back() characters.  reserve() if you know in advance how many you need (and then you can directly access elements g_buf[i] without push_back, but they will be overwritten by any push_back or resize operations.
// of course, using this buffer is not thread-safe.
//extern CharBuf g_buf;

// FIXME: use thread-local storage instead of static ... reentrance stack?
template <class T>
struct default_bufsize {
  enum { BUFSIZE=16 };
};

template <>
struct default_bufsize<char> {
  enum { BUFSIZE=CHARBUF_INIT_SIZE };
};

template <class T>
struct static_buf {
  typedef DynamicArray<T> Buf;
  static Buf buf;
  //operator Buf &() { return buf; }
  Buf & operator ()() const { return buf; }
// FIXME: not reentrant
  static_buf() { buf.clear(); }
};

typedef static_buf<char> char_buf;

#ifdef MAIN
template<class T>
typename static_buf<T>::Buf static_buf<T>::buf(default_bufsize<T>::BUFSIZE);

CharBuf g_buf(CHARBUF_INIT_SIZE);
#endif

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( charbuf )
{
  BOOST_CHECK((char_buf())().size() == 0);
  {
  char_buf b;
  BOOST_CHECK(b().size()==0);
  b().push_back('a');
  BOOST_CHECK(b()[0]=='a');
  BOOST_CHECK((char_buf())().size() == 0);
  }

    BOOST_CHECK((char_buf())().size() == 0);
  {
  char_buf b;
  BOOST_CHECK(b().size()==0);
  b().push_back('a');
  BOOST_CHECK(b()[0]=='a');
  BOOST_CHECK((char_buf())().size() == 0);
  }

}
#endif

#endif
