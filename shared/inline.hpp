/** \file

    force inlining on or off:

    ALWAYS_INLINE void f() { g(); }
    NEVER_INLINE void f2() { g(); }
*/

#ifndef INLINE_JG_2014_11_12_HPP
#define INLINE_JG_2014_11_12_HPP

#ifndef ALWAYS_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif

#ifndef NEVER_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define NEVER_INLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
#define NEVER_INLINE __declspec(noinline)
#else
#define NEVER_INLINE
#endif
#endif

#endif
