// sprinkle "BACKTRACE;" (macro) in strategic locations, and BackTrace::print() inside exception handler.
#ifndef BACKTRACE_HPP
#define BACKTRACE_HPP


#ifndef GRAEHL__BACKTRACE_MAIN
# ifdef GRAEHL__SINGLE_MAIN
#  define GRAEHL__BACKTRACE_MAIN
# endif
#endif

#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/dynarray.h>
#include <exception>
#include <iostream>

#ifdef __linux__
#include <signal.h>
#include <execinfo.h>
#endif

namespace graehl {

static const int MAX_TRACE_DEPTH=64;

void print_stackframe(std::ostream &o) {
#ifdef HAVE_LINUX_BACKTRACE
    void *trace[MAX_TRACE_DEPTH];

    int trace_size = ::backtrace(trace, MAX_TRACE_DEPTH);
    char **messages = ::backtrace_symbols(trace, trace_size);
    o << "!!Stack backtrace:\n";
    for (int i=0; i<trace_size; ++i)
        o << "!! " << messages[i] << std::endl;
#endif
}

class BackTrace
{
    const char *function;
    const char *file;
    unsigned line;
    static bool first;
public:
    struct Loc {
        const char *file;
        const char *function;
        unsigned line;
        Loc(const char *fun,const char *fil,unsigned lin) : file(fil),function(fun),line(lin) {}
        template <class O>
        void print(O &o) const {
            o << function << "() [" << file << ":" << line << "]";
        }
        typedef Loc self_type;
        TO_OSTREAM_PRINT
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
    template <class O>
    static void print(O& o) {
        if (stack.size()) {
            o.clear();
            o << "Exception unwind sequence:\n";
            stack.print_multiline(o);
            o << std::endl;
        }
    }
    void print()
    {
        print(std::cerr);
    }
    typedef BackTrace self_type;
    TO_OSTREAM_PRINT
};

#ifdef GRAEHL__BACKTRACE_MAIN
BackTrace::LocStack BackTrace::stack;
bool BackTrace::first=true;
#endif

} //graehl


//defined(DEBUG) ||
#if !defined(NO_BACKTRACE)
#define BACKTRACE graehl::BackTrace BackTrace_line_ ## __LINE__(__FUNCTION__,__FILE__,__LINE__)
#else
#define BACKTRACE
#endif

#endif
