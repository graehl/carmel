#ifndef SHELL_JG2012615_HPP
#define SHELL_JG2012615_HPP

#include <graehl/shared/shell_escape.hpp>
#include <graehl/shared/os.hpp>

namespace graehl {

inline void copy_file(const std::string &source, const std::string &dest, bool skip_same_size_and_time=false)
{
  const char *rsync="rsync -qt";
  const char *cp="/bin/cp -p";
  std::stringstream s;
  s << (skip_same_size_and_time ? rsync : cp) << ' ';
  out_shell_quote(s,source);
  s << ' ';
  out_shell_quote(s,dest);
// INFOQ("copy_file",s.str());
  system_safe(s.str());
}

inline void mkdir_parents(const std::string &dirname)
{
  const char *mkdir="/bin/mkdir -p ";
  std::stringstream s;
  s << mkdir;
  out_shell_quote(s,dirname);
  system_safe(s.str());
}

inline int system_shell_safe(const std::string &cmd)
{
  const char *shell="/bin/sh -c ";
  std::stringstream s;
  s << shell;
  out_shell_quote(s,cmd);
  return system_safe(s.str());
}

}

#endif // SHELL_JG2012615_HPP
