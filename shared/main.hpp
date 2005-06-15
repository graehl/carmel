// some main() boilerplate
#ifndef MAIN_HPP
#define MAIN_HPP

#include "config.h"
#include <locale>
#include <iostream>

#ifdef SYNC_STDIO
# define UNSYNC_STDIO
# else
# define UNSYNC_STDIO std::ios::sync_with_stdio(false);std::cin.tie(0);
#  endif

#define MAIN_DECL int MAINDECL main(int argc, char *argv[])

#ifdef _MSC_VER
#define MAINDECL __cdecl

#define MAIN_BEGIN MAIN_DECL { MainGuard _mg;   UNSYNC_STDIO;

#define MAIN_END }

struct MainGuard {
  INITLEAK_DECL
    unsigned i;
  MainGuard() : i(0) {
        INITLEAK_DO;
        std::ios::sync_with_stdio(false);
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
#define MAIN_BEGIN MAIN_DECL {  UNSYNC_STDIO;

#define MAIN_END }

#endif



#endif
