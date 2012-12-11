#ifndef GRAEHL_SHARED__HAVE_64_BITS_HPP
#define GRAEHL_SHARED__HAVE_64_BITS_HPP

#ifndef HAVE_64_BITS

// Check windows
#if defined(_WIN32) || defined(_WIN64)
#if defined(_WIN64)
#define HAVE_64_BITS 1
#else
#define HAVE_64_BITS 0
#endif
#endif

// Check GNU
#if __GNUC__
#if __x86_64__ || __ppc64__
#define HAVE_64_BITS 1
#else
#define HAVE_64_BITS 0
#endif
#endif

#endif // HAVE_64_BITS

#endif
