#ifndef GRAEHL_SHARED__IFDBG_HPP
#define GRAEHL_SHARED__IFDBG_HPP

#include <graehl/shared/os.hpp>

#define SHV(x) " "#x<<"="<<x

#ifndef GRAEHL_IFDBG_ENABLE
//uncomment to enable IFDBG for release (NDEBUG) also
//#define GRAEHL_IFDBG_ENABLE 1
#endif

#ifndef GRAEHL_IFDBG_ENABLE
//# define GRAEHL_IFDBG_ENABLE !defined(NDEBUG)
# ifndef NDEBUG
#  define GRAEHL_IFDBG_ENABLE 1
# else
#  define GRAEHL_IFDBG_ENABLE 0
# endif
#endif

#define MACRO_NOT_NULL(IF) (0 IF(||1))

#define DECLARE_DBG_LEVEL_C(n,env) DECLARE_ENV_C_LEVEL(n,getenv_##env,env)
#define DECLARE_DBG_LEVEL(ch) DECLARE_DBG_LEVEL_C(ch##_DBG_LEVEL,ch##_DBG)
#define DECLARE_DBG_LEVEL_IF(ch) ch(DECLARE_DBG_LEVEL_C(ch##_DBG_LEVEL,ch##_DBG))

#if GRAEHL_IFDBG_ENABLE
# define IFDBG(ch,l) if(MACRO_NOT_NULL(ch) && ch##_DBG_LEVEL>=(l))
#else
# define IFDBG(ch,l) if(0)
#endif

// ch(IFDBG...) so channel need not be declared if noop #define ch(x)
#define EIFDBG(ch,l,e) do { ch(IFDBG(ch,l) { e; }) }while(0)

//show.hpp
#define SHOWIF0(ch,l,m) EIFDBG(ch,l,SHOWM0(ch,#ch": " m))
#define SHOWIF1(ch,l,m,x0) EIFDBG(ch,l,SHOWM1(ch,#ch": " m,x0))
#define SHOWIF2(ch,l,m,x0,x1) EIFDBG(ch,l,SHOWM2(ch,#ch": " m,x0,x1))
#define SHOWIF3(ch,l,m,x0,x1,x2) EIFDBG(ch,l,SHOWM3(ch,#ch": " m,x0,x1,x2))

#endif
