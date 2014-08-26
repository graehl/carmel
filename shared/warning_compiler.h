#ifndef GRAEHL_SHARED__WARNING_COMPILER_HPP
#define GRAEHL_SHARED__WARNING_COMPILER_HPP


#define EXPAND_PRAGMA_STR(s) #s
#define EXPAND_PRAGMA_JOINSTR(x, y) EXPAND_PRAGMA_STR(x ## y)
#define EXPAND_PRAGMA(x) _Pragma (#x)
#define GCC_DIAG_PRAGMA(x) EXPAND_PRAGMA(GCC diagnostic x)
#define CLANG_DIAG_PRAGMA(x) EXPAND_PRAGMA(clang diagnostic x)

#if __clang__
# define CLANG_DIAG_IGNORE(x) CLANG_DIAG_PRAGMA(ignored EXPAND_PRAGMA_JOINSTR(-W, x))
# define CLANG_DIAG_OFF(x) CLANG_DIAG_PRAGMA(push) CLANG_DIAG_IGNORE(x)
# define CLANG_DIAG_ON(x) CLANG_DIAG_PRAGMA(pop)
#else
# define CLANG_DIAG_ON(x)
# define CLANG_DIAG_OFF(x)
# define CLANG_DIAG_IGNORE(x)
#endif

#if !__clang__ && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 2)
# define HAVE_GCC_DIAG_OFF 1
# define GCC_DIAG_IGNORE(x) GCC_DIAG_PRAGMA(ignored EXPAND_PRAGMA_JOINSTR(-W, x))
# define GCC_DIAG_WARN(x) GCC_DIAG_PRAGMA(warning EXPAND_PRAGMA_JOINSTR(-W, x))
# if __GNUC_MINOR__ >= 6
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) GCC_DIAG_IGNORE(x)
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_IGNORE(x)
#  define GCC_DIAG_ON(x)  GCC_DIAG_WARN(x)
# endif
#endif

#if !__clang__ && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 4)
# define HAVE_GCC_4_4 1
#else
# define HAVE_GCC_4_4 0
#endif

#if !__clang__ && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 6)
# define HAVE_GCC_4_6 1
# define GCC46_DIAG_OFF(x) GCC_DIAG_OFF(x)
# define GCC46_DIAG_ON(x) GCC_DIAG_ON(x)
# define PRAGMA_EITHER(x) EXPAND_PRAGMA(GCC x)
# define PRAGMA_GCC_CLANG(gcc, clang) EXPAND_PRAGMA(GCC gcc)
# define HAVE_DIAGNOSTIC_PUSH 1
# define HAVE_PRAGMA_EITHER 1
# define HAVE_PRAGMA_GCC_DIAGNOSTIC 1
#else
# define HAVE_GCC_4_6 0
# define GCC46_DIAG_OFF(x)
# define GCC46_DIAG_ON(x)
# if __clang__
#  define PRAGMA_EITHER(x) EXPAND_PRAGMA("clang "#x)
#  define PRAGMA_GCC_CLANG(gcc, clang) EXPAND_PRAGMA("clang "#clang)
#  define HAVE_PRAGMA_EITHER 1
#  define HAVE_DIAGNOSTIC_PUSH 1
# else
#  define PRAGMA_EITHER(x)
#  define PRAGMA_GCC_CLANG(gcc, clang)
#  define HAVE_PRAGMA_EITHER 0
#  define HAVE_DIAGNOSTIC_PUSH 0
# endif
#endif


#if !__clang__ && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 8)
# define HAVE_GCC_4_8 1
#else
# define HAVE_GCC_4_8 0
#endif

#define DIAGNOSTIC_PUSH() PRAGMA_EITHER(diagnostic push)

#define DIAGNOSTIC_PRAGMA_WARNING_EITHER(x) \
  PRAGMA_EITHER(diagnostic ignored EXPAND_PRAGMA_JOINSTR(-W, x))

#define DIAGNOSTIC_PRAGMA_WARNING_GCC_CLANG(x, y) \
  PRAGMA_GCC_CLANG(diagnostic ignored EXPAND_PRAGMA_JOINSTR(-W, x), \
                   diagnostic ignored EXPAND_PRAGMA_JOINSTR(-W, y))

#define DIAGNOSTIC_WARNING_PUSH(x) DIAGNOSTIC_PUSH() DIAGNOSTIC_PRAGMA_WARNING_EITHER(x)

#define DIAGNOSTIC_POP() PRAGMA_EITHER(diagnostic pop)

#define PRAGMA_GCC_DIAGNOSTIC HAVE_DIAGNOSTIC_PUSH

#if !HAVE_GCC_DIAG_OFF
# define HAVE_GCC_DIAG_OFF 0
# define GCC_DIAG_IGNORE(x)
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
#endif

#endif
