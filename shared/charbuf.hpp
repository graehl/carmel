#ifndef _CHARBUF_HPP
#define _CHARBUF_HPP
// no separate implementation for now, just #define MAIN in one source file that includes this
#include "../carmel/src/dynarray.h"
#ifndef CHARBUF_INIT_SIZE
#define CHARBUF_INIT_SIZE 4096
#endif


typedef DynamicArray<char> CharBuf;

// USAGE: clear() before you use, repeatedly push_back() characters.  reserve() if you know in advance how many you need (and then you can directly access elements g_buf[i] without push_back, but they will be overwritten by any push_back or resize operations.  
// of course, using this buffer is not thread-safe.
extern CharBuf g_buf;

#ifdef MAIN
CharBuf g_buf(CHARBUF_INIT_SIZE);
#endif

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( charbuf )
{
}
#endif

#endif
