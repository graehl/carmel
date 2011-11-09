#ifndef GRAEHL_SHARED__MAIN_HPP
#define GRAEHL_SHARED__MAIN_HPP

// main() boilerplate

#include <graehl/shared/stream_util.hpp>
#include <locale>
#include <iostream>


namespace graehl {

inline void default_locale()
{
    std::locale::global(std::locale(""));
}

#ifndef UNSYNC_STDIO
# define UNSYNC_STDIO 1
#else

#ifndef DEFAULT_LOCALE
# define DEFAULT_LOCALE 1
#endif

#if UNSYNC_STDIO
# define UNSYNC_STDIO graehl::unsync_cout()
#else
# define UNSYNC_STDIO
#endif

#if DEFAULT_LOCALE
# define NO_LOCALE default_locale()
#else
# define NO_LOCALE
#endif

#define MAIN_DECL int main(int argc, char *argv[])

#define MAIN_BEGIN MAIN_DECL { UNSYNC_STDIO; NO_LOCALE;

#define MAIN_END }

}//graehl

#endif
