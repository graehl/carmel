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

#ifdef MAIN
#define BOOST_AUTO_TEST_MAIN
#endif

//!< if i starts with the first character of the header string, copy rest of
//line (but not newline) to o; otherwise, write the header string to o
inline void copy_header(std::istream &i, std::ostream &o,const char *header_string="$$$") {
    char c;
    if (i.get(c)) {        
        if (c==header_string[0]) {
            do {
                o.put(c);
            } while(i.get(c) && c!='\n');
            return;
        } else 
            i.unget();
    }
    o << header_string;
}

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
