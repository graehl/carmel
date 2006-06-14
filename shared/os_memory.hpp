#ifndef GRAEHL__SHARED__OS_MEMORY_HPP
#define GRAEHL__SHARED__OS_MEMORY_HPP

// via start-with-limit.c from Jens-S. Vöckler

#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <ext/stdio_filebuf.h>
#include <graehl/shared/size_mega.hpp>
#include <stdexcept>
#include <string>
#include <sstream>

#ifdef GRAEHL__OS_MEMORY_POPEN
#else 
#include <sys/sysinfo.h>
#endif

namespace graehl {

using __gnu_cxx::stdio_filebuf;

struct os_memory 
{
    typedef unsigned long long memory_size;
    memory_size ram, swap;
    os_memory() 
    {
        measure();
    }
    static void throw_error(std::string const& reason) 
    {
        throw std::runtime_error((reason+": ")+strerror(errno));
    }
    static void unix_nofail(int ret,std::string const& call)
    {
        if (ret == -1)
            throw_error(call);
    }
    
    
    void measure() 
    {
#ifdef GRAEHL__OS_MEMORY_POPEN
        char line[4096];
        using namespace std;
        FILE* free;
        if ( (free=popen( "/usr/bin/free -b", "r" )) == NULL )
            throw_error("popen /usr/bin/free");
        char* s;
        ram = swap = -1llu;
        // std::istream in(stdio_filebuf(free));
        while ( fgets( line, sizeof(line), free ) ) {
            if ( tolower(line[0]) == 'm' && line[1] == 'e' && line[2] == 'm' ) {
                /* RAM */
                s = line;
                while ( *s && ! isdigit(*s) ) ++s;
                ram = strtoull( s, 0, 10 );
            } else if ( tolower(line[0]) == 's' && line[1] == 'w' && line[2] == 'a' ) {
                /* swap */
                s = line;
                while ( *s && ! isdigit(*s) ) ++s;
                swap = strtoull( s, 0, 10 );
            }
        }
        pclose(free);
#else
        struct sysinfo info;
        unix_nofail(sysinfo(&info),"sysinfo");
        ram=info.mem_unit*info.totalram;
        swap=info.mem_unit*info.totalswap;
#endif 
    }

    // without reference to how much memory already used, advise physical+swap_portion*swap-swap_reserve_megs
    memory_size suggest_limit(double swap_portion=0,double swap_reserve_bytes=0) 
    {
        return (memory_size)(ram+(swap_portion*swap-swap_reserve_bytes));
    }

    struct printable_limit 
    {
        printable_limit(memory_size amount) : amount(amount) {}
        memory_size amount;
        template <class O>
        void print(O&o) const 
        {
            if (amount == RLIM_INFINITY)
                o << "INFINITY";
            else
                o << size_bytes(amount);
        }        
        template <class O>
        friend
        O & operator<<(O& o,printable_limit const& me) 
        {
            me.print(o);
            return o;
        }        
    };
    
        
    // also RLIMIT_STACK (man getrlimit)
    static memory_size set_memory_limit(memory_size limit,std::ostream *log=NULL,int rlim_type=RLIMIT_AS)
    {
        rlimit l;
        /* obtain current address space (virtual memory) limits */
        unix_nofail(getrlimit( rlim_type, &l ),"getrlimit");

        if ( l.rlim_max != RLIM_INFINITY && l.rlim_max < limit ) {
            /* new limit too high, ignore it */
            limit=l.rlim_max;
            if (log) *log << "Requested limit "<<printable_limit(limit) << " exceeds hard limit of "<<printable_limit(l.rlim_max) << " - reducing to meet hard limit." << std::endl;
        }
        /* set new limit */
        l.rlim_cur = limit;
        unix_nofail(setrlimit( rlim_type, &l ),"setrlimit");
        /* verify - never, ever trust Linux */
        unix_nofail(getrlimit( rlim_type, &l),"getrlimit");
        
        if ( l.rlim_cur != limit ) {
            if (log) *log << "Didn't get requested soft limit " << printable_limit(limit) << "; got " << printable_limit(l.rlim_cur)<<std::endl;
        } else {
            if (log) *log << "Succesfully set soft limit to " << printable_limit(limit) << " (hard limit: " << printable_limit(l.rlim_max) << ")" << std::endl;
        }
        
        return l.rlim_cur;        
    }
    
    template <class O>
    void print(O&o) const 
    {
        o << "mem="<<size_bytes(ram)<< " swap="<<size_bytes(swap);
    }
    template <class O>
    friend
    O & operator<<(O& o,os_memory const& me) 
    {
        me.print(o);
        return o;
    }
};

    
}

#ifdef SAMPLE
# define OS_MEMORY_SAMPLE
#endif

#ifdef OS_MEMORY_SAMPLE
# include <iostream>
int main(int argc, char *argv[])
{
    using namespace std;
    using namespace graehl;
    
    os_memory mem;
    cout << mem << endl;
    if (argc==2) {
        mem.set_memory_limit(size_bytes_integral(string(argv[1]),true),&cerr);
    } else if (argc==3) {
        size_bytes swap_portion(argv[1],true);
        size_bytes swap_reserve_bytes(argv[2],true);
        size_bytes_integral lim=mem.suggest_limit(swap_portion,swap_reserve_bytes);
        cerr << "asking for limit " << lim << " from swap portion " << swap_portion << " and reserved for system swap of " << swap_reserve_bytes <<"\n";
        mem.set_memory_limit(lim,&cerr);
    }
    return 0;
}

#endif 

#endif
