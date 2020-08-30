// Copyright 2014 Jonathan Graehl-http://graehl.org/
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
#ifndef SHELL_JG2012615_HPP
#define SHELL_JG2012615_HPP
#pragma once

#include <boost/filesystem.hpp>
#include <graehl/shared/os.hpp>
#include <graehl/shared/shell_escape.hpp>

namespace graehl {

inline void copy_file(std::string const& source, std::string const& dest, bool skip_same_size_and_time = false) {
  char const* rsync = "rsync -qt";
  char const* cp = "/bin/cp -p";
  std::stringstream s;
  s << (skip_same_size_and_time ? rsync : cp) << ' ';
  out_shell_quote(s, source);
  s << ' ';
  out_shell_quote(s, dest);
  // INFOQ("copy_file", s.str());
  system_safe(s.str());
}

inline void mkdir_parents(std::string const& dirname) {
  boost::filesystem::create_directories(dirname);
}

inline int system_shell_safe(std::string const& cmd) {
  char const* shell = "/bin/sh -c ";
  std::stringstream s;
  s << shell;
  out_shell_quote(s, cmd);
  return system_safe(s.str());
}


}

#endif
