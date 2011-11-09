#ifndef GRAEHL_SHARED__MAIN_HPP
#define GRAEHL_SHARED__MAIN_HPP
// some main() boilerplate w/ microsoft leak detection, which is obsolete w/ valgrind

#include <graehl/shared/config.h>
#include <graehl/shared/stream_util.hpp>
#include <locale>
#include <iostream>

#ifdef _MSC_VER
# include <graehl/shared/memleak.hpp>
#endif

namespace graehl {

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
