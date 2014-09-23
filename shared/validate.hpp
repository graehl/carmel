#ifndef GRAEHL_SHARED__VALIDATE_HPP
#define GRAEHL_SHARED__VALIDATE_HPP

// separated from configure.hpp so you can include just this in your headers (assuming you don't prefer to write your own validator fns and want to use these

#include <boost/filesystem.hpp>
#include <graehl/shared/verbose_exception.hpp>
#include <graehl/shared/from_strings.hpp>
#include <vector>
#include <map>
#include <boost/optional.hpp>

namespace configure {

SIMPLE_EXCEPTION_PREFIX(config_exception, "configure: ");

template <class I>
struct bounded_range_validate
{
  I begin, end;
  std::string desc;
  bounded_range_validate(I const& begin, I const& end, std::string const& desc)
    : begin(begin), end(end), desc(desc) {}
  template <class I2>
  void operator()(I2 const& i2)
  {
    if (i2<begin || !(i2<end))
      throw config_exception(desc+" value "+graehl::to_string(i2)+" - should have ["+graehl::to_string(begin)+" <= value <  "+graehl::to_string(end)+")");
  }
};

template <class I>
bounded_range_validate<I> bounded_range(I const& begin, I const& end, std::string const& desc="value out of bounds") {
  return bounded_range_validate<I>(begin, end, desc); }

template <class I>
struct bounded_range_inclusive_validate
{
  I begin, end;
  std::string desc;
  bounded_range_inclusive_validate(I const& begin, I const& end, std::string const& desc)
    : begin(begin), end(end), desc(desc) {}
  template <class I2>
  void operator()(I2 const& i2)
  {
    if (i2<begin || end<i2)
      throw config_exception(desc+" value "+graehl::to_string(i2)+" - should have ["+graehl::to_string(begin)+" <= value <=  "+graehl::to_string(end)+"]");
  }
};

template <class I>
bounded_range_inclusive_validate<I> bounded_range_inclusive(I const& begin, I const& end, std::string const& desc="value out of bounds") {
  return bounded_range_inclusive_validate<I>(begin, end, desc); }

struct exists
{
  template <class Path>
  void operator()(Path const& pathname)
  {
    boost::filesystem::path path(pathname);
    if (!boost::filesystem::exists(path)) throw config_exception(path.string()+" not found.");
  }
};

struct dir_exists
{
  template <class Path>
  void operator()(Path const& pathname)
  {
    boost::filesystem::path path(pathname);
    if (!is_directory(path)) throw config_exception("directory "+path.string()+" not found.");
  }
};

struct file_exists
{
  template <class Path>
  void operator()(Path const& pathname)
  {
    boost::filesystem::path path((pathname));
    if (!boost::filesystem::exists(path)) throw config_exception("file "+path.string()+" not found.");
    if (is_directory(path)) throw config_exception(path.string()+" is a directory. Need a file.");
  }
};

template <class Val>
struct one_of
{
  std::vector<Val> allowed;
  one_of(std::vector<Val> const& allowed) : allowed(allowed) {}
  one_of(one_of const& o) : allowed(o.allowed) {}
  one_of &operator()(Val const& v) { allowed.push_back(v); }
  template <class Key>
  void operator()(Key const& key) const
  {
    if (std::find(allowed.begin(), allowed.end(), key)==allowed.end())
      throw config_exception(to_string(key)+" not allowed - must be one of "+to_string(*this));
  }
  friend inline std::string to_string(one_of<Val> const& one)
  {
    return "["+graehl::to_string_sep(one.allowed, "|")+"]";
  }
};


}

namespace adl {

template <class T>
void validate(T &, void *lowerPriorityMatch = 0)
{
}

/// you can call this from outside our namespace to get the more specific version via ADL if it exists
template <class T>
void adl_validate(T &t)
{
  validate(t);
}

}

namespace std {
template <class T>
void validate(std::vector<T> const& v)
{
  for (typename std::vector<T>::const_iterator i=v.begin(), e=v.end();i!=e;++i)
    adl::adl_validate(*i);
}
template <class Key, class T>
void validate(std::map<Key, T> const& v)
{
  for (typename std::map<Key, T>::const_iterator i=v.begin(), e=v.end();i!=e;++i)
    adl::adl_validate(i->second);
}
}

namespace boost {
template <class T>
void validate(boost::optional<T> const& i)
{
  if (i)
    adl::adl_validate(*i);


}}

#endif
