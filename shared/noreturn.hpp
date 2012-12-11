#ifndef NORETURN_JG2012613_HPP
#define NORETURN_JG2012613_HPP

#if defined(__GNUC__) && __GNUC__>=3
# define NORETURN __attribute__ ((noreturn))
#else
# define NORETURN
#endif

#endif // NORETURN_JG2012613_HPP
