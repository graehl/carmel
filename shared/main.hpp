#ifndef MAIN_HPP
#define MAIN_HPP

#include "config.h"
#include <locale>
#include <iostream>

#ifdef _MSC_VER
#define MAINDECL __cdecl
#else
#define MAINDECL
#endif

#define MAIN_DECL int MAINDECL main(int argc, char *argv[])

#define MAIN_BEGIN MAIN_DECL { MainGuard _mg;

#define MAIN_END }

inline std::ostream & print_cmdline(std::ostream &o,int argc, char *argv[]) {
    for (int i=0;i<argc;++i) {
        if (i)
            o << ' ';
        o << argv[i];
    }
    return o;
}

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
