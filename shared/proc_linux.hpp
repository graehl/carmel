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
#ifndef GRAEHL_SHARED__PROC_LINUX_HPP
#define GRAEHL_SHARED__PROC_LINUX_HPP

#include <iostream>
#include <fstream>
#include <graehl/shared/string_match.hpp>
#include <graehl/shared/string_to.hpp>

namespace graehl {
#ifdef __linux__
#define PROCNAME(x) static char const* x = #x
PROCNAME(State);
PROCNAME(SleepAVG);
PROCNAME(Tgid);
PROCNAME(Pid);
PROCNAME(PPid);
PROCNAME(TracerPid);
PROCNAME(FDSize);
PROCNAME(Groups);
PROCNAME(Uid);
PROCNAME(Gid);
PROCNAME(VmPeak);
PROCNAME(VmSize);
PROCNAME(VmHWM);
PROCNAME(VmRSS);
PROCNAME(VmData);
PROCNAME(VmStk);
PROCNAME(VmExe);
PROCNAME(VmLib);
PROCNAME(VmPTE);
PROCNAME(StaBrk);
PROCNAME(StaStk);
PROCNAME(Brk);
PROCNAME(Threads);
//there are more ...
#undef PROCNAME

namespace {
const std::string kB="kB";
}

// doesn't wokr for stabrak/brk/stastk which are in hex
inline double proc_bytes(std::string const& val) {
#if 0
  if (ends_with(val, kB))
#else
    std::size_t s = val.size();
  if (s>2 && val[s-1]=='B' && val[s-2]=='k')
#endif
    return 1024.*string_to<double>(std::string(val.begin(), val.end()-2));
  else
    return string_to<double>(val);
}


inline std::string get_proc_field(std::string const& field, std::istream &in, bool required = true) {
  using namespace std;
  string s;
  while (std::getline(in, s)) {
    if (starts_with(s, field)) {
      size_t fs = field.size();
      if (s.size()>fs && s[fs]==':')
        return trim(std::string(s.begin()+fs+1, s.end()));
    }
  }
  if (required)
    throw std::runtime_error("Couldn't find '"+field+":' line in /proc file.");
  else
    return string();
}

inline std::string get_proc_field(std::string const& field, std::string const& filename="/proc/self/status", bool required = true) {
  std::ifstream i(filename.c_str());
  return get_proc_field(field, i, required);
}
#endif
}


#endif
