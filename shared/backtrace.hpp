#ifndef _BACKTRACE_HPP
#define _BACKTRACE_HPP

#include "dynarray.h"
#include <exception>

class BackTrace
{
    typedef std::pair<char * const, unsigned> Loc;
    typedef DynamicArray<Loc> LocStack;
    static LocStack stack;
    char * const fileName;
    unsigned     line;
public:
    BackTrace(char * const f, unsigned l)
        : file(f)
        , line(l)
        {}
    ~BackTrace()
        {
            if (std::uncaught_exception())
                {
                    stack.push_back(Loc(file, line ));
                }
        }
    static void print(std::ostream& o)
        {
            o.clear();
            o << "Backtrace (top = stack top):\n" << stack;
        }
};

#ifdef MAIN
static BackTrace::LocStack BackTrace::stack;
#endif

#define BACKTRACE StackTrace BackTrace72845389034(__FILE__,__LINE__)

#endif
