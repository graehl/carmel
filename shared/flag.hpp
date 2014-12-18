#ifndef FLAG_JG201276_HPP
#define FLAG_JG201276_HPP
#pragma once

#include <boost/detail/atomic_count.hpp>
#include <graehl/shared/string_to.hpp>
#include <string>

namespace graehl {

//default-false bool
struct flag
{
  bool v;
  void set(bool val) { v = val; }
  bool get() const { return v; }
  operator bool() const { return v; }
  operator bool& () { return v; }
  bool* operator &() { return &v; }
  bool const* operator &() const { return &v; }
  bool first() {
    bool r = !v;
    v = true;
    return r;
  }
  friend inline bool latch(flag &once) {
    return once.first();
  }
  void finish() {
    v = true;
  }
  void clear() {
    v = false;
  }
  /** \return if first() has already returned true (peeks without modifying state) */
  inline bool already() const {
    return v;
  }
  flag() : v() {}
  flag(bool v) : v(v) {}
  flag(flag const& o) : v(o.v) {}
  flag& operator = (bool newv)
  {
    v = newv;
    return *this;
  }
  typedef void leaf_configure;
  typedef void assign_bool; //TODO: debug in context of configure_program_options flag() and value_str - bad any_cast (from bool, presumably)
  friend std::string to_string_impl(flag const& x) { return to_string(x.v); }
  friend void string_to_impl(std::string const& str, flag & x) { string_to(str, x.v); }
  friend std::string type_string(flag const&) { return "boolean"; } //TODO: ADL
};

using boost::detail::atomic_count;
/**
   unlike flag or default_true, this is thread-safe with Size = atomic_count (for unsafe: use std::size_t)
*/
template <class Size = atomic_count>
struct counter
{
  friend inline std::ostream& operator<<(std::ostream &out, counter const& self) {
    self.print(out);
    return out;
  }
  void print(std::ostream &out) const {
    out << v;
  }
  typedef Size size_type;
  size_type v;
  void set(std::size_t val) { v = size_type(val); }
  std::size_t get() const { return (std::size_t)v; }
  operator std::size_t() const { return (std::size_t)v; }
  operator size_type& () { return v; }
  size_type* operator &() { return &v; }
  size_type const* operator &() const { return &v; }
  bool first() {
    return ++v == 1;
  }
  counter() : v() {}
  counter(std::size_t v) : v(v) {}
  counter& operator = (std::size_t newv)
  {
    v = size_type(newv);
    return *this;
  }
  typedef void leaf_configure;
  friend std::string to_string_impl(counter const& x) { return to_string(x.v); }
  friend void string_to_impl(std::string const& str, counter & x) { string_to(str, x.v); }
  friend std::string type_string(counter const& x) { return "count"; } //TODO: ADL
};

struct default_true
{
  bool v;
  void set(bool val) { v = val; }
  bool get() const { return v; }
  operator bool() const { return v; }
  operator bool& () { return v; }
  bool* operator &() { return &v; }
  bool const* operator &() const { return &v; }
  default_true() : v(true) {}
  default_true(bool v) : v(v) {}
  default_true(default_true const& o) : v(o.v) {}
  default_true& operator = (bool newv)
  {
    v = newv;
    return *this;
  }
  bool first() {
    bool r = v;
    v = false;
    return r;
  }
  typedef void leaf_configure;
  typedef void assign_bool; //TODO: debug in context of configure_program_options default_true() and value_str - bad any_cast (from bool, presumably)
  friend std::string to_string_impl(default_true const& x) { return to_string(x.v); }
  friend void string_to_impl(std::string const& str, default_true & x) { string_to(str, x.v); }
  friend std::string type_string(default_true const&) { return "boolean"; } //TODO: ADL
};


}

#endif
