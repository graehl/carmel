#ifndef GRAEHL_SHARED__AUTO_REPORT_HPP
#define GRAEHL_SHARED__AUTO_REPORT_HPP

#include <iostream>

namespace graehl {

template <class Change>
struct auto_report
{
  Change change;
  operator Change & () { return change; }
  std::ostream *o;
  std::string desc;
  bool reported;
  auto_report(std::ostream &o,std::string const& desc=Change::default_desc())
    : o(&o),desc(desc),reported(false) {}
  auto_report(std::ostream *o=NULL,std::string const& desc=Change::default_desc())
    : o(o),desc(desc),reported(false) {}
  void set(std::ostream &out,std::string const& descr=Change::default_desc())
  {
    o=&out;
    desc=descr;
    reported=false;
  }
  void report(bool nl=true)
  {
    if (o) {
      *o << desc << change;
      if (nl)
        *o << std::endl;
      else
        *o << std::flush;
      reported=true;
    }
  }
  ~auto_report()
  {
    if (!reported)
      report();
  }
};

}


#endif
