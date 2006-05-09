#ifndef GRAEHL__SHARED__TIME_SPACE_REPORT_HPP
#define GRAEHL__SHARED__TIME_SPACE_REPORT_HPP

#include <graehl/shared/memory_stats.hpp>
#include <graehl/shared/stopwatch.hpp>

namespace graehl {

struct time_space_report : public memory_report
{
    stopwatch time;
    std::string new_desc;
    time_space_report(std::ostream &o,std::string const& desc_="memory/time elapsed: ")
        : memory_report(o,""),new_desc(desc_) {}
    void report()
    {
        o << new_desc;
        o << time.recent_total_time() << "sec, memory ";
        memory_report::report();
    }
    ~time_space_report()
    {
        if (!reported)
            report();
    }
};

}


#endif
