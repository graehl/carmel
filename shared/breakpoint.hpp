#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#ifndef BREAKPOINT

#ifdef _MSC_VER
# define BREAKPOINT __asm int 3
#else
# ifdef __i386__
#  define BREAKPOINT asm("   int $3")
# else
#  define BREAKPOINT   *(int *)0 = 0
# endif
#endif

#endif

#ifdef DEBUG
# define DEBUG_BREAKPOINT BREAKPOINT
#else
# define DEBUG_BREAKPOINT 
#endif


#endif
