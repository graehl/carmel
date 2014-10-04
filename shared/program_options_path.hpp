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
