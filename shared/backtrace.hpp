#ifndef _BACKTRACE_HPP
#define _BACKTRACE_HPP

#include "dynarray.h"
#include <exception>
#include <iostream>

class BackTrace
{
    typedef std::pair<const char *, unsigned> Loc;
    typedef DynamicArray<Loc> LocStack;
    static LocStack stack;
    const char *file;
    unsigned line;
    struct LocWriter {
        std::ostream & operator()(std::ostream &o,const Loc &loc) {
            return o << loc.first << " line " << loc.second;
        }
    };
public:
    BackTrace(const char *f, unsigned l)  : file(f), line(l) {}
    ~BackTrace() {
        if (std::uncaught_exception())
            stack.push_back(Loc(file, line ));
    }
    static void print_on(std::ostream& o=std::cerr) {
        if (stack.size()) {
            o.clear();
            o << "Exception unwind sequence:\n";
            stack.print_on(o,LocWriter(),LocStack::MULTILINE);
            o << std::endl;
        }
    }
};

#ifdef MAIN
BackTrace::LocStack BackTrace::stack;
#endif

#if defined(DEBUG) || !defined(NO_BACKTRACE)
#define BACKTRACE BackTrace BackTrace72845389034(__FILE__,__LINE__)
#else
#define BACKTRACE
#endif

#endif
