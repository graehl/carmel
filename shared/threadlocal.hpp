#ifndef GRAEHL_SHARED__THREADLOCAL_HPP
#define GRAEHL_SHARED__THREADLOCAL_HPP

/** \file

    add thread safety to the thread-unsafe trick of saving/restoring globals in local holders to simulate dynamic scoping.

    this implementation relies on the correctness of compiler thread-local attributes for globals. this isn't safe across DLLs in windows.

    also, destructors won't necessarily be called for thread specific data. so use THREADLOCAL only on pointers and plain-old-data
*/

#ifndef GRAEHL_SETLOCAL_SWAP
# define GRAEHL_SETLOCAL_SWAP 0
#endif

#include <boost/utility.hpp> // for BOOST_NO_MT

#ifdef BOOST_NO_MT
# define THREADLOCAL
#else

# ifdef _MSC_VER
//MSVC - see http://msdn.microsoft.com/en-us/library/9w1sdazb.aspx
///WARNING: doesn't work with DLLs ... use boost thread specific pointer instead (http://www.boost.org/libs/thread/doc/tss.html)
#  define THREADLOCAL __declspec(thread)
# else
#  if __APPLE__
//APPLE: __thread even with gcc -pthread isn't allowed:
/* quoting mailing list:

  __thread is not supported. The specification is somewhat ELF-
  specific, which makes it a bit of a challenge.
*/
#   define THREADLOCAL
#  else
//GCC - see http://gcc.gnu.org/onlinedocs/gcc-4.6.2/gcc/Thread_002dLocal.html
#   define THREADLOCAL __thread
#  endif
# endif

#endif


namespace graehl {

template <class D>
struct SaveLocal {
    D &value;
    D old_value;
    SaveLocal(D& val) : value(val), old_value(val) {}
    ~SaveLocal() {
#if GRAEHL_SETLOCAL_SWAP
      using namespace std;
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
#if GRAEHL_SETLOCAL_SWAP
      new_value
#else
      val
#endif
      ) {
#if GRAEHL_SETLOCAL_SWAP
      using namespace std;
      swap(value,old_value);
#else
      value=new_value;
#endif
    }
    ~SetLocal() {
#if GRAEHL_SETLOCAL_SWAP
      using namespace std;
      swap(value,old_value);
#else
      value=old_value;
#endif
    }
};


}

#endif
