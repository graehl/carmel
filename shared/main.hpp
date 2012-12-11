#ifndef GRAEHL_SHARED__MAIN_HPP
#define GRAEHL_SHARED__MAIN_HPP

// main() boilerplate

#ifndef GRAEHL_USE_BOOST_LOCALE
# define GRAEHL_USE_BOOST_LOCALE 0
#endif

#if GRAEHL_USE_BOOST_LOCALE
# include <boost/locale/util.hpp>
#endif

#include <graehl/shared/stream_util.hpp>
#include <locale>
#include <iostream>

namespace graehl {

inline void default_locale()
{
  try {
  // mac doesn't like empty LC_ALL
#if GRAEHL_USE_BOOST_LOCALE
  std::string sysLocale=boost::locale::util::get_system_locale(true);
  boost::locale::generator localeGen;
  std::locale::global(localeGen(sysLocale));
  ::setlocale(LC_ALL, sysLocale.c_str());
#else
    std::locale::global(std::locale(""));
    ::setlocale(LC_ALL, "");
#endif
  } catch(std::exception &e) {
    std::cerr<<"Couldn't set default locale (compare your LC_ALL against `locale -a`): "<<e.what()<<"\n";
  }
}

#ifndef USE_UNSYNC_STDIO
# define USE_UNSYNC_STDIO 1
#endif

#ifndef HAVE_DEFAULT_LOCALE
# define HAVE_DEFAULT_LOCALE 1
#endif

#if USE_UNSYNC_STDIO
# define UNSYNC_STDIO graehl::unsync_cout()
#else
# define UNSYNC_STDIO
#endif

#if HAVE_DEFAULT_LOCALE
# define NO_LOCALE default_locale()
#else
# define NO_LOCALE
#endif

#define MAIN_DECL int main(int argc, char *argv[])

#define MAIN_BEGIN MAIN_DECL { NO_LOCALE; UNSYNC_STDIO;

#define MAIN_END }

}//graehl

#endif
