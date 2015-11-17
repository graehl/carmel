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
