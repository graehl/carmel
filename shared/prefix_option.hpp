#ifndef GRAEHL_SHARED__PREFIX_OPTION_HPP
#define GRAEHL_SHARED__PREFIX_OPTION_HPP

/* for boost program options with opt="long-name,l" "prefix-" =>
 * "prefix-long-name" - short option is stripped to avoid conflict */

#include <string>

namespace graehl {

inline std::string prefix_option(std::string opt,std::string const& prefix="")
{
  if (prefix.empty()) return opt;
  std::string::size_type nopt=opt.size();
  if (nopt>2 && opt[nopt-2]==',')
    opt.resize(nopt-2);
  return prefix+opt;
}

inline std::string suffix_option(std::string opt,std::string const& suffix="")
{
  if (suffix.empty()) return opt;
  std::string::size_type nopt=opt.size();
  if (nopt>2 && opt[nopt-2]==',')
    opt.resize(nopt-2);
  return opt+suffix;
}

}//ns

#endif
