// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef GRAEHL_SHARED__AUTO_REPORT_HPP
#define GRAEHL_SHARED__AUTO_REPORT_HPP
#pragma once

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
  auto_report(std::ostream &o, std::string const& desc=Change::default_desc())
    : o(&o), desc(desc), reported(false) {}
  auto_report(std::ostream *o=NULL, std::string const& desc=Change::default_desc())
    : o(o), desc(desc), reported(false) {}
  void set(std::ostream &out, std::string const& descr=Change::default_desc())
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
        *o << '\n';
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
