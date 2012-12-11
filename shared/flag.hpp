#ifndef FLAG_JG201276_HPP
#define FLAG_JG201276_HPP

//#include <graehl/shared/string_to.hpp>
//#include <graehl/shared/type_string.hpp>
#include <string>

namespace graehl {

//default-false bool
struct flag
{
  bool v;
  operator bool() const { return v; }
  operator bool& () { return v; }
  bool* operator &() { return &v; }
  bool const* operator &() const { return &v; }
  flag() : v() {}
  flag(bool v) : v(v) {}
  flag(flag const& o) : v(o.v) {}
  flag& operator=(bool newv)
  {
    v=newv;
    return *this;
  }
  typedef void leaf_configure;
  typedef void assign_bool; //TODO: debug in context of configure_program_options flag() and value_str - bad any_cast (from bool, presumably)
  friend std::string to_string_impl(flag const& x) { return to_string(x.v); }
  friend void string_to_impl(std::string const& str,flag & x) { string_to(str,x.v); }
  friend std::string type_string(flag const& x) { return "boolean"; } //TODO: ADL
};

struct default_true
{
  bool v;
  operator bool() const { return v; }
  operator bool& () { return v; }
  bool* operator &() { return &v; }
  bool const* operator &() const { return &v; }
  default_true() : v(true) {}
  default_true(bool v) : v(v) {}
  default_true(default_true const& o) : v(o.v) {}
  default_true& operator=(bool newv)
  {
    v=newv;
    return *this;
  }
  typedef void leaf_configure;
  typedef void assign_bool; //TODO: debug in context of configure_program_options default_true() and value_str - bad any_cast (from bool, presumably)
  friend std::string to_string_impl(default_true const& x) { return to_string(x.v); }
  friend void string_to_impl(std::string const& str,default_true & x) { string_to(str,x.v); }
  friend std::string type_string(default_true const& x) { return "boolean"; } //TODO: ADL
};

}

#endif // DEFAULT_TRUE_JG201276_HPP
