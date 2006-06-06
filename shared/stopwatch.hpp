#ifndef __STOPWATCH_HPP__
#define __STOPWATCH_HPP__

#include <stdexcept>
#include <sys/time.h>
#include <sys/resource.h>
#include <graehl/shared/stream_util.hpp>

namespace graehl {

class stopwatch
{
 public:
    enum timer_type {
        WALL_TIME,
        TOTAL_TIME,
        SYSTEM_TIME,
        USER_TIME,
        PAGEFAULTS,
        TYPE_MAX
    };
#define EACH_TIMER_TYPE(i) for (int i=WALL_TIME;i<TYPE_MAX;i++)
    explicit stopwatch(bool start_running=true) : running(start_running)
    {
        reset(start_running);
    }
    void start() 
    {
        running = true;
        measure_usage(then_usage);
        measure_wallclock(then_wallclock);
    }
    
    /// pauses
    void stop() 
    {
        if (running)
            EACH_TIMER_TYPE(i)
                totals[i] = total_time((timer_type)i);
        running = false;
    }

    //clears
    void reset(bool start_running=true) 
    {
        EACH_TIMER_TYPE(i)
            totals[i]=0;
        if (start_running)
            start();
    }
    
#undef EACH_TIMER_TYPE
    static bool valid_type(timer_type type) 
    {
        return type >= WALL_TIME && type < TYPE_MAX;
    }
    template <class Time>
    static double sec(const Time &t) 
    {
        return t.tv_sec+t.tv_usec/(1000000.);
    }
    template <class Time>
    static double elapsed_sec(const Time &now,const Time &then) 
    {
        return sec(now)-sec(then);
    }
    static void measure_usage(rusage &r) 
    {
        if (-1==getrusage (RUSAGE_SELF, &r))
            throw std::runtime_error("getrusage failed");
    }
    static void measure_wallclock(timeval &tv) 
    {
        if (-1==gettimeofday(&tv, 0))
            throw std::runtime_error("gettimeofday failed");
    }
    double recent_user_time() const
    {
        if (!running) return 0;
        rusage now_usage;
        measure_usage(now_usage);
        return elapsed_sec(now_usage.ru_utime,then_usage.ru_utime);
    }
    double recent_system_time() const
    {
        if (!running) return 0;
        rusage now_usage;
        measure_usage(now_usage);
        return elapsed_sec(now_usage.ru_stime,then_usage.ru_stime);
    }
    double recent_total_time() const
    {
        return recent_user_time() + recent_system_time();
    }
    double recent_wall_time() const
    {
        if (!running) return 0;
        timeval now_wallclock;
        measure_wallclock(now_wallclock);
        return elapsed_sec(now_wallclock,then_wallclock);
    }
    double recent_major_pagefaults() const
    {
        if (!running) return 0;
        rusage now_usage;
        measure_usage(now_usage);
        return now_usage.ru_majflt-then_usage.ru_majflt;
    }
    double total_time(timer_type type) const
    {
        if (!valid_type(type))
            throw std::runtime_error("stopwatch: invalid timer type");
        return totals[type] + recent_time(type);
    }
    double recent_time(timer_type type) const
    {
        if (!valid_type(type))
            throw std::runtime_error("stopwatch: invalid timer type");
        if (!running) return 0;
        switch (type) { 
        case WALL_TIME:
            return recent_wall_time();
        case TOTAL_TIME:
            return recent_total_time();
        case USER_TIME:
            return recent_user_time();
        case SYSTEM_TIME:
            return recent_system_time();
        case PAGEFAULTS:
            return recent_major_pagefaults();
        default:
            throw std::runtime_error("bug - didn't handle timer type in stopwatch::recent_time");
        }
    }

    template <class Ostream>
    void print(Ostream &os) const 
    {
        os << '[' << total_time(WALL_TIME) << " wall sec, " << total_time(USER_TIME) << " user sec, " << total_time(SYSTEM_TIME) << " system sec, " << total_time(PAGEFAULTS) << " major page faults]";
    }
    typedef stopwatch self_type;
    TO_OSTREAM_PRINT
    template <class Ostream>
    friend Ostream & operator <<(Ostream &os,stopwatch const& t) {
        t.print(os);    
        return os;
    }
  
 private:
    bool running;

    rusage then_usage;
    timeval then_wallclock;

    double totals[TYPE_MAX];    
//    std::vector<double> totals;
};

}//graehl

#endif

