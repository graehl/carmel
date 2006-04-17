//encapsulates the thread-unsafe trick of saving/restoring globals/statics so that functions using them are reentrant (sometimes I prefer to use globals instead of recursively passing the same constant arguments)
#ifndef THREADLOCAL_HPP
#define THREADLOCAL_HPP

#ifdef BOOST_NO_MT

#define THREADLOCAL

#else

#ifdef _MSC_VER
//FIXME: doesn't work with DLLs ... use TLS apis instead (http://www.boost.org/libs/thread/doc/tss.html)
#define THREADLOCAL __declspec(thread)
#else

#if 1
#define THREADLOCAL
#else
#define THREADLOCAL __thread
#endif

#endif

#endif

#include <boost/utility.hpp>

template <class D>
struct SaveLocal {
    D &value;
    D old_value;
    SaveLocal(D& val) : value(val), old_value(val) {}
    ~SaveLocal() {
#ifdef SETLOCAL_SWAP
      swap(value,old_value);
#else
      value=old_value;
#endif
    }
};

template <class D>
struct SetLocal {
    D &value;
    D old_value;
    SetLocal(D& val,const D &new_value) : value(val), old_value(
#ifdef SETLOCAL_SWAP
      new_value
#else
      val
#endif
      ) {
#ifdef SETLOCAL_SWAP
      swap(value,old_value);
#else
      value=new_value;
#endif
    }
    ~SetLocal() {
#ifdef SETLOCAL_SWAP
      swap(value,old_value);
#else
      value=old_value;
#endif
    }
};

#ifdef TEST
#include <graehl/shared/test.hpp>

//typedef LocalGlobal<int> Gint;
typedef int Gint;
static Gint savelocal_n=1;

/*
BOOST_AUTO_UNIT_TEST( threadlocal )
{
  BOOST_CHECK(savelocal_n==1);
  {
    SaveLocal<int> a(savelocal_n);
    savelocal_n=2;
    BOOST_CHECK(savelocal_n==2);
    {
      SetLocal<int> a(savelocal_n,3);
      BOOST_CHECK(savelocal_n==3);
    }
    BOOST_CHECK(savelocal_n==2);

  }
  BOOST_CHECK(savelocal_n==1);
}
*/
#endif

#endif



