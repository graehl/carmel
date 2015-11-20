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
/** \file for boost program options with opt="long-name,l" "prefix-" =>
"prefix-long-name" - short option is stripped to avoid conflict */

#ifndef GRAEHL_SHARED__PREFIX_OPTION_HPP
#define GRAEHL_SHARED__PREFIX_OPTION_HPP
#pragma once


#include <string>

namespace graehl {

inline std::string prefix_option(std::string opt, std::string const& prefix = "") {
  if (prefix.empty()) return opt;
  std::string::size_type nopt = opt.size();
  if (nopt > 2 && opt[nopt - 2] == ',') opt.resize(nopt - 2);
  return prefix + opt;
}

inline std::string suffix_option(std::string opt, std::string const& suffix = "") {
  if (suffix.empty()) return opt;
  std::string::size_type nopt = opt.size();
  if (nopt > 2 && opt[nopt - 2] == ',') opt.resize(nopt - 2);
  return opt + suffix;
}


}

#endif
