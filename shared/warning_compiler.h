#ifndef WARNING_COMPILER_HPP
#define WARNING_COMPILER_HPP


#ifndef HAVE_GCC_4_6
# if __GNUC__ > 4 || __GNUC__==4 && __GNUC_MINOR__ > 6
#  define PRAGMA_EITHER(x) _Pragma("GCC "#x)
#  define PRAGMA_GCC_CLANG(gcc,clang) _Pragma("GCC "#gcc)
#  define HAVE_GCC_4_6 1
#  define HAVE_DIAGNOSTIC_PUSH 1
#  define HAVE_PRAGMA_EITHER 1
# else
#  define HAVE_GCC_4_6 0
#  if __clang__
#   define PRAGMA_EITHER(x) _Pragma("clang "#x)
#   define PRAGMA_GCC_CLANG(gcc,clang) _Pragma("clang "#clang)
#   define HAVE_PRAGMA_EITHER 1
#   define HAVE_DIAGNOSTIC_PUSH 1
#  else
#   define PRAGMA_EITHER(x)
#   define PRAGMA_GCC_CLANG(gcc,clang)
#   define HAVE_PRAGMA_EITHER 0
#   define HAVE_DIAGNOSTIC_PUSH 0
#  endif
# endif
#endif

#if HAVE_GCC_4_6
# define HAVE_GCC_4_4 1
#else
# ifndef HAVE_GCC_4_4
#  undef HAVE_DIAGNOSTIC_PUSH
#  if __GNUC__ > 4 || __GNUC__==4 && __GNUC_MINOR__ > 3
#   define HAVE_GCC_4_4 1
#   define HAVE_DIAGNOSTIC_PUSH 0
    // weird, they took pragma diagnostic out of 4.5?
#  else
#   define HAVE_GCC_4_4 0
#   define HAVE_DIAGNOSTIC_PUSH 0
#  endif
# endif
#endif


// these don't work because _Pragma doesn't allow preprocessor string interpretation? perhaps use stringize?
#define DIAGNOSTIC_PUSH() PRAGMA_EITHER("diagnostic push")

#define DIAGNOSTIC_PRAGMA_WARNING_EITHER(x) PRAGMA_EITHER("diagnostic warning "#x)
#define DIAGNOSTIC_PRAGMA_WARNING_GCC_CLANG(x,y) PRAGMA_GCC_CLANG("diagnostic warning "#x,"diagnostic warning "#y)

#define DIAGNOSTIC_WARNING_PUSH(x) DIAGNOSTIC_PUSH() DIAGNOSTIC_PRAGMA_WARNING_EITHER(x)

#define DIAGNOSTIC_POP() PRAGMA_EITHER("diagnostic pop")

#define PRAGMA_GCC_DIAGNOSTIC HAVE_DIAGNOSTIC_PUSH

#endif
