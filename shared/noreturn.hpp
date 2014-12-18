/** \file

    avoid no-return-value compiler warnings for infinite loops and throw that
    will never return from a fn
*/

#ifndef NORETURN_JG2012613_HPP
#define NORETURN_JG2012613_HPP
#pragma once

#if defined(__GNUC__) && __GNUC__>=3
# define NORETURN __attribute__ ((noreturn))
#elif defined(__clang__)
# define ANALYZER_NORETURN _attribute__((analyzer_noreturn))
# define NORETURN
#else
# define NORETURN
#endif

#ifndef ANALYZER_NORETURN
# define ANALYZER_NORETURN
#endif

#endif
