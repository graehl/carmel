#ifndef MAIN_HPP
#define MAIN_HPP

#include "config.h"
#include <locale>

#ifdef _MSC_VER
#define MAINDECL __cdecl
#else
#define MAINDECL
#endif

#define MAIN int MAINDECL main(int argc, char *argv[])

#define MAIN_BEGIN MAIN { MainGuard _mg;

#define MAIN_END }

struct MainGuard {
    MainGuard() {
        INITLEAK;
        std::cin.tie(0);
//        std::locale::global(std::locale(""));
    }
    ~MainGuard() {
        CHECKLEAK(0);
    }
};


#endif
