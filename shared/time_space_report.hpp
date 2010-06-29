#ifndef GRAEHL__SHARED__TIME_SPACE_REPORT_HPP
#define GRAEHL__SHARED__TIME_SPACE_REPORT_HPP

#include <graehl/shared/time_report.hpp>
#include <graehl/shared/memory_stats.hpp>

namespace graehl {

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

typedef auto_report<time_space_change> time_space_report;

}


#endif
