// global logging facility
#ifndef GRAEHL__SHARED__GLOG_HPP
#define GRAEHL__SHARED__GLOG_HPP
//GLOBAL LOGGING
//TODO: crib from Debug.h, debugprint.h, use teestream

#include <graehl/shared/threadlocal.hpp>
#include <graehl/shared/myassert.h>
#include <sstream>
#include <graehl/shared/byref.hpp>
#include <iostream>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <graehl/shared/print.hpp>
#include <graehl/shared/byref.hpp>

#ifdef DEBUG
#define DEBUG_SEGFAULT Assert(0)
#else
#define DEBUG_SEGFAULT
#endif

namespace graehl {

namespace glog {
    extern unsigned depth;
    extern bool disable;
    extern int current_chat;
    extern int chat_level;
    extern std::ostream *logstream;
    struct scopedepth {
        scopedepth() { ++depth;}
        ~scopedepth() { --depth;}
    };
    inline bool is_enabled() {
        return (!disable && current_chat <= chat_level && logstream);
    }
    void print_indent();
    void print_indent(std::ostream &o);
    void set_loglevel(int loglevel=0);
    void set_logstream(std::ostream &o=std::cerr);
#ifdef GRAEHL__SINGLE_MAIN
    int current_chat;
    int chat_level;
    std::ostream *logstream=&std::cerr;
    void print_indent() {
        for(unsigned i=0;i<depth;++i)
            DBPS(" ");
    }
    void print_indent(std::ostream &o) {
        for(unsigned i=0;i<depth;++i)
            o << ' ';
    }
    void set_loglevel(int loglevel){
        chat_level=loglevel;
    }
    void set_logstream(std::ostream *o) {
        logstream=o;
    }

    unsigned depth=0;
    bool disable=false;
#endif
}

}

#endif
