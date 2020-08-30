#pragma once
#ifndef GRAEHL_CPP11

#if __cplusplus >= 201700L
#define GRAEHL_CPP17 1
#else
#define GRAEHL_CPP17 0
#endif

#if __cplusplus >= 201103L || SDL_CPP11 || _MSC_VER >= 1900
#define GRAEHL_CPP11 1
#if __cplusplus >= 201400L
#define GRAEHL_CPP14 1
#define GRAEHL_CPP14_TYPETRAITS 1
#else
#define GRAEHL_CPP14 0
#define GRAEHL_CPP14_TYPETRAITS 0
#endif
#else
#define GRAEHL_CPP11 0
#define GRAEHL_CPP14 0
#define GRAEHL_CPP14_TYPETRAITS 0
#endif

#if GRAEHL_CPP11
#define GRAEHL_CONSTEXPR constexpr
#else
#define GRAEHL_CONSTEXPR
#endif

#if _MSC_VER >= 1900
#undef GRAEHL_CPP14_TYPETRAITS
#define GRAEHL_CPP14_TYPETRAITS 1
#endif

#if __cplusplus >= 201700L
// GCC 8.2 has 201709 and clang 7.0 has 201707
#define GRAEHL_CPP17 1
#else
#define GRAEHL_CPP17 0
#endif

#if __cplusplus >= 202000L
// GCC 8.2 has 201709 and clang 7.0 has 201707
#define GRAEHL_CPP20 1
#else
#define GRAEHL_CPP20 0
#endif

#endif
