// some main() boilerplate
#ifndef MAIN_HPP
#define MAIN_HPP

#include <graehl/shared/config.h>
#include <locale>
#include <iostream>

#ifdef _MSC_VER
# include <graehl/shared/memleak.hpp>
#endif

namespace graehl {

inline void unsync_cout()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);
}

inline void default_locale()
{
    std::locale::global(std::locale(""));
}

#ifdef SYNC_STDIO
# define UNSYNC_STDIO
#else
# define UNSYNC_STDIO graehl::unsync_cout()
#endif

#define MAIN_DECL int MAINDECL main(int argc, char *argv[])

#ifdef _MSC_VER
#define MAINDECL __cdecl
#else
#define MAINDECL
#endif

struct MainGuard {
#ifdef _MSC_VER
  INITLEAK_DECL
#endif
  unsigned i;
  MainGuard() : i(0) {
#ifdef _MSC_VER
        INITLEAK_DO;
#endif 
        unsync_cout();
//        default_locale();
    }
  void checkpoint_memleak() {
#ifdef _MSC_VER      
    CHECKLEAK(i);
#endif 
    ++i;
  }
    ~MainGuard() {
    }
};

#define MAIN_BEGIN MAIN_DECL { graehl::MainGuard _mg;   UNSYNC_STDIO;

#define MAIN_END }


}//graehl

#endif
