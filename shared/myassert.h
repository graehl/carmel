//workaround for difficulties I've had getting a debugger breakpoint from a failed assert
#ifndef GRAEHL_SHARED__ASSERT_H
#define GRAEHL_SHARED__ASSERT_H


#include <graehl/shared/breakpoint.hpp>
#include <graehl/shared/config.h>

#ifdef DEBUG

#define GRAEHL_ASSERT 1
#endif
#ifndef GRAEHL_ASSERT
# define GRAEHL_ASSERT 0
#endif

#ifndef DEBUG
# ifndef NDEBUG
//# define NDEBUG
#endif
#endif

#include <cassert>


inline static void _my_assert(const char *file,unsigned line,const char *expr)
{
  Config::err() << file << "(" << line << ") Assertion failed: " << expr << std::endl;
  BREAKPOINT;
}

template <class T>
inline static void _my_assert(const char *file,unsigned line,const T&t,const char *expr,const char *expect)
{
  Config::err() << file << "(" << line << ") Assertion failed: (" << expr << ") was " << t << "; expected " << expect << std::endl;
  BREAKPOINT;
}

#if GRAEHL_ASSERT
# define Assert(expr) (expr) ? (void)0 :   \
                 _my_assert(__FILE__,__LINE__,#expr)
// WARNING: expr occurs twice (repeated computation)
# define Assert2(expr,expect) do {                                                        \
        /* Config::log() << #expr << ' ' << #expect << " = " << (expr expect) << std::endl;*/   \
        if (!((expr) expect)) _my_assert(__FILE__,__LINE__,expr,#expr,#expect);                 \
            } while(0)
# define Paranoid(a) do { a; } while (0)
#else
# define Assert(a)
# define Assert2(a,b)
# define Paranoid(a)
#endif

#endif
