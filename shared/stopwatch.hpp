#ifndef GRAEHL__SHARED__STOPWATCH_HPP
#define GRAEHL__SHARED__STOPWATCH_HPP

/*
  stopwatch can track recent and total elapsed time and pagefaults.

  Windows emulation of [recent_]wall_time() and total_time() - note: total = user+system
  - most appropriate for benchmarking.
 */

#include <stdexcept>

#ifdef _WIN32


#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef VC_EXTRALEAN
# define VC_EXTRALEAN 1
#endif
#ifndef NOMINMAX
# define NOMINMAX 1
#endif
#include <windows.h>
#undef min
#undef max
#undef DELETE
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <graehl/shared/print_read.hpp>

namespace graehl {

#ifdef _WIN32
struct timeval {
   long    tv_sec;         /* seconds */
   long    tv_usec;        /* and microseconds */
};
#endif


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
#ifndef _WIN32
        measure_usage(then_usage);
#endif
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
#ifndef _WIN32
    static void measure_usage(rusage &r)
    {
        if (-1==getrusage (RUSAGE_SELF, &r))
            throw std::runtime_error("getrusage failed");
    }
#endif
    static void measure_wallclock(timeval &tv)
    {
        if (-1==gettimeofday(&tv, 0))
            throw std::runtime_error("gettimeofday failed");
    }

    double recent_user_time() const
    {
#ifndef _WIN32
        if (!running) return 0;
        rusage now_usage;
        measure_usage(now_usage);
        return elapsed_sec(now_usage.ru_utime,then_usage.ru_utime);
#else
		return 0; // TODO: implement Win32 version
#endif
    }
    double recent_system_time() const
    {
#ifndef _WIN32
        if (!running) return 0;
        rusage now_usage;
        measure_usage(now_usage);
        return elapsed_sec(now_usage.ru_stime,then_usage.ru_stime);
#else
		return 0; // TODO: implement Win32 version
#endif
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
#ifndef _WIN32
        if (!running) return 0;
        rusage now_usage;
        measure_usage(now_usage);
        return now_usage.ru_majflt-then_usage.ru_majflt;
#else
		return 0; // TODO: implement Win32 version
#endif
    }

    double total_time(timer_type type=TOTAL_TIME) const
    {
        if (!valid_type(type))
            throw std::runtime_error("stopwatch: invalid timer type");
        return totals[type] + recent_time(type);
    }
    double recent_time(timer_type type=TOTAL_TIME) const
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

#ifdef _WIN32

	static int gettimeofday (struct timeval *tv, void* tz)
    {
        union {
            long long ns100; /*time since 1 Jan 1601 in 100ns units */
            FILETIME ft;
        } now;

        GetSystemTimeAsFileTime (&now.ft);
        tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
        tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
        return 0;
}
#endif

    bool running;

#ifndef _WIN32
    rusage then_usage;
#endif
    timeval then_wallclock;

    double totals[TYPE_MAX];
};

}//graehl

#endif
