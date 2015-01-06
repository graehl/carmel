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
/** \file

    .
*/

#ifndef PROGRAM_OPTIONS_PATH_JG_2014_10_03_HPP
#define PROGRAM_OPTIONS_PATH_JG_2014_10_03_HPP

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
# define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_FILESYSTEM_VERSION
# define BOOST_FILESYSTEM_VERSION 3
#endif
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

namespace boost {
namespace filesystem {

void validate(boost::any& value,
              std::vector<std::string> const& args,
              boost::filesystem::path*, int)
{
  po::validators::check_first_occurrence(value);
  std::string const& val = po::validators::get_single_string(args);
  value = boost::filesystem::path(val);
}

void validate(boost::any& value,
              std::vector<std::string> const& args,
              std::vector<boost::filesystem::path>*, int)
{
  po::validators::check_first_occurrence(value);
  typedef std::vector<boost::filesystem::path> Paths;
  std::size_t i = 0, N = args.size();
  Paths paths(N);
  for (; i < N; ++i)
    paths[i] = args[i];
  value = paths;
}

}}


#endif
