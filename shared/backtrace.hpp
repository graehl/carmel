// sprinkle "BACKTRACE;" (macro) in strategic locations, and BackTrace::print() inside exception handler.
#ifndef BACKTRACE_HPP
#define BACKTRACE_HPP

#include <graehl/shared/dynarray.h>
#include <exception>
#include <iostream>

#ifdef LINUX_BACKTRACE
#include <signal.h>
#include <execinfo.h>
#endif

#define MAX_TRACE_DEPTH 64

void print_stackframe(std::ostream &o) {
#ifdef LINUX_BACKTRACE
    void *trace[MAX_TRACE_DEPTH];

    int trace_size = backtrace(trace, MAX_TRACE_DEPTH);
    char **messages = backtrace_symbols(trace, trace_size);
    o << "!!Stack backtrace:\n";
    for (int i=0; i<trace_size; ++i)
        o << "!! " << messages[i] << std::endl;
#endif
}

class BackTrace
{
    const char *file;
    const char *function;
    unsigned line;
    static bool first;
public:
    struct Loc {
        const char *file;
        const char *function;
        unsigned line;
        Loc(const char *fun,const char *fil,unsigned lin) : file(fil),function(fun),line(lin) {}
        void print(std::ostream &o) const {
            o << function << "() [" << file << ":" << line << "]";
        }
        friend std::ostream &operator <<(std::ostream &o,const Loc &l);
    };
    typedef dynamic_array<Loc> LocStack;
    static LocStack stack;
    BackTrace(const char *fun,const char *fil, unsigned l)  : function(fun),file(fil), line(l) {}
    ~BackTrace() {
        if (std::uncaught_exception()) {
            stack.push_back(Loc(function,file, line ));
            if (first) {
                print_stackframe(std::cerr);
                first=false;
            }
        }
    }
    static void print(std::ostream& o=std::cerr) {
        if (stack.size()) {
            o.clear();
            o << "Exception unwind sequence:\n";
            stack.print_multiline(o);
            o << std::endl;
        }
    }
};

std::ostream &operator <<(std::ostream &o,const BackTrace::Loc &l)
{
    l.print(o);
    return o;
}


#ifdef SINGLE_MAIN
BackTrace::LocStack BackTrace::stack;
bool BackTrace::first=true;
#endif

//defined(DEBUG) ||
#if !defined(NO_BACKTRACE)
#define BACKTRACE BackTrace BackTrace_line_ ## __LINE__(__FUNCTION__,__FILE__,__LINE__)
#else
#define BACKTRACE
#endif

#endif
