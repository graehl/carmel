#ifndef MAIN_HPP
#define MAIN_HPP

#include "config.h"
#include <locale>
#include <iostream>

#define MAIN_DECL int MAINDECL main(int argc, char *argv[])

#ifdef _MSC_VER
#define MAINDECL __cdecl

#define MAIN_BEGIN MAIN_DECL { MainGuard _mg;

#define MAIN_END }

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
#else
#define MAINDECL
#define MAIN_BEGIN MAIN_DECL { 

#define MAIN_END }

#endif



#endif
