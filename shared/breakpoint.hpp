#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#ifndef BREAKPOINT

#if defined(_MSC_VER) && defined(_WIN32)
# define BREAKPOINT __asm int 3
#else
# if defined(__i386__) || defined(__x86_64__)
#  define BREAKPOINT asm("int $3")
# else
#  define BREAKPOINT   do{ volatile int *p=0;*p=0;} while(0)
# endif
#endif

#endif

#ifdef DEBUG
# define DEBUG_BREAKPOINT BREAKPOINT
#else
# define DEBUG_BREAKPOINT
#endif


#endif
