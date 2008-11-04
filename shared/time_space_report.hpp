#ifndef GRAEHL__SHARED__TIME_SPACE_REPORT_HPP
#define GRAEHL__SHARED__TIME_SPACE_REPORT_HPP

#include <graehl/shared/auto_report.hpp>
#include <graehl/shared/memory_stats.hpp>
#include <graehl/shared/stopwatch.hpp>

namespace graehl {

struct time_change 
{
    static char const* default_desc() 
    { return "\nelapsed: "; }
    
    stopwatch time;
    template <class O>
    void print(O &o) const
    {
        o << const_cast<stopwatch&>(time).recent_total_time() << " sec";
    }
    typedef time_change self_type;

    TO_OSTREAM_PRINT    
};

struct time_space_change 
{
    static char const* default_desc() 
    { return "\ntime and memory used: "; }
    time_change tc;
    memory_change mc;
    void print(std::ostream &o) const
    {
        o << tc << ", memory " << mc;
    }
    typedef time_space_change self_type;
    TO_OSTREAM_PRINT
};

typedef auto_report<time_change> time_report;
typedef auto_report<time_space_change> time_space_report;

}


#endif
