/** \file

    macros for module-specific debugging (to be disabled at compile time depending on build) - see also
   show.hpp

    usage:

    #ifdef NDEBUG
    # define MODULE_IFDBG(x)
    #else
    # define MODULE_IFDBG(x) x
    #endif
*/

#ifndef GRAEHL_SHARED__IFDBG_HPP
#define GRAEHL_SHARED__IFDBG_HPP

#ifndef GRAEHL_IFDBG_ENABLE
#ifdef NDEBUG
// release
#define GRAEHL_IFDBG_ENABLE 0
#else
#define GRAEHL_IFDBG_ENABLE 1
#endif
#endif

#define MACRO_NOT_NULL(IF) (0 IF(|| 1))

#if GRAEHL_IFDBG_ENABLE
#define IFDBG(ch, l) if (MACRO_NOT_NULL(ch) && ch##_DBG_LEVEL >= (l))
#else
#define IFDBG(ch, l) if (0)
#endif

// ch(IFDBG...) so channel need not be declared if noop #define ch(x)
#define EIFDBG(ch, l, e)    \
  do {                      \
    ch(IFDBG(ch, l) { e; }) \
  } while (0)

// for show.hpp
#define SHOWIF0(ch, l, m) EIFDBG(ch, l, SHOWM0(ch, #ch ": " m))
#define SHOWIF1(ch, l, m, x0) EIFDBG(ch, l, SHOWM1(ch, #ch ": " m, x0))
#define SHOWIF2(ch, l, m, x0, x1) EIFDBG(ch, l, SHOWM2(ch, #ch ": " m, x0, x1))
#define SHOWIF3(ch, l, m, x0, x1, x2) EIFDBG(ch, l, SHOWM3(ch, #ch ": " m, x0, x1, x2))
#define SHOWIF4(ch, l, m, x0, x1, x2, x3) EIFDBG(ch, l, SHOWM4(ch, #ch ": " m, x0, x1, x2, x3))
#define SHOWIF5(ch, l, m, x0, x1, x2, x3, x4) EIFDBG(ch, l, SHOWM5(ch, #ch ": " m, x0, x1, x2, x3, x4))

#endif
