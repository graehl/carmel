#ifndef GRAEHL__SHARED__TIME_REPORT_HPP
#define GRAEHL__SHARED__TIME_REPORT_HPP
#pragma once

#ifndef USE_BOOST_TIMER_TIMER
#if (BOOST_VERSION / 100000)>=1 && ((BOOST_VERSION/100)%1000)>=48
# define USE_BOOST_TIMER_TIMER 1
#else
# define USE_BOOST_TIMER_TIMER 0
#endif
#endif

#include <graehl/shared/auto_report.hpp>
#if USE_BOOST_TIMER_TIMER
//# include <boost/time/cpu_timer.hpp>
# ifndef BOOST_SYSTEM_NO_DEPRECATED
#  define BOOST_SYSTEM_NO_DEPRECATED 1
# endif
# include <boost/config.hpp>
# include <boost/chrono/chrono.hpp>
# include <boost/timer/timer.hpp>
#else
# include <graehl/shared/stopwatch.hpp>
#endif
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/print_width.hpp>

namespace graehl {

struct time_change
{
  static char const* default_desc()
  { return "\nelapsed: "; }
#if USE_BOOST_TIMER_TIMER
  boost::timer::cpu_timer time;
  double elapsed_wall() const
  {
    return time.elapsed().wall*.000000001;
  }
  double elapsed() const
  {
    boost::timer::cpu_times t=time.elapsed();
    return .000000001*(t.system+t.user);
  }
  void start() {time.start();}
  void resume() {time.resume();}
#else
  stopwatch time;
  void start() {time.reset();}
  void resume() {time.start();}
  double elapsed() const {
    return const_cast<stopwatch&>(time).total_time();
  }
#endif
  void stop() {time.stop();}
  void reset()
  {
#if USE_BOOST_TIMER_TIMER
    start();
    stop();
#else
    time.reset(false);
#endif
  }

  template <class O>
  void print(O &o) const
  {
    print_width(o, elapsed(),6) << " sec";
  }

  typedef time_change self_type;
  TO_OSTREAM_PRINT
};

typedef auto_report<time_change> time_report;


}

#endif
