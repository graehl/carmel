#ifndef MAIN_HPP
#define MAIN_HPP

#include "config.h"
#include <locale>

#ifdef _MSC_VER
#define MAINDECL __cdecl
#else
#define MAINDECL
#endif

#define MAIN_DECL int MAINDECL main(int argc, char *argv[])

#define MAIN_BEGIN MAIN_DECL { MainGuard _mg;

#define MAIN_END }

#ifdef MAIN
#define BOOST_AUTO_TEST_MAIN
#endif

struct MainGuard {
  INITLEAK_DECL
    unsigned i;
  MainGuard() : i(0) {
        INITLEAK_DO;
        std::cin.tie(0);
//        std::locale::global(std::locale(""));
    }
  void checkpoint_memleak() {
    CHECKLEAK(i);
    ++i;
  }
    ~MainGuard() {
    }
};


#endif
