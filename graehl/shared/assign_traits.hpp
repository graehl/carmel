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
/** \file

    for configure.hpp library - when operator= isn't what you want.

// usage: graehl::assign_traits<T>::assign/init(...)
// or you can override one or both of (ADL/friend) init_impl/assign_impl

// rationale: lacking proper traits for (public) default construct and assign, turn compile time error into
run time error for generic code that *may* init or assign.

//TODO: C++11 std::is_assignable<T>-see
http://compgroups.net/comp.lang.c++.moderated/determining-whether-type-is-assignabl/204746

// boost::any is involved here only to prevent compilation of any_cast -> assign for types that don't support
it. any_cast requires a copy constructor (returns by value)

*/

#ifndef GRAEHL_SHARED__ASSIGN_TRAITS_JG_HPP
#define GRAEHL_SHARED__ASSIGN_TRAITS_JG_HPP
#pragma once

#include <boost/any.hpp>
#include <graehl/shared/cpp11.hpp>
#include <memory>
#include <stdexcept>
#if GRAEHL_CPP11
#include <utility>
#endif

namespace graehl {


template <class T>
void init_impl(T& t) {
  t.~T();
  new (&t) T();
}

template <class T>
void assign_impl(T& t, T const& from) {
  t = from;
}

#if GRAEHL_CPP11
template <class T>
void assign_impl(T& t, T&& from) {
  t = std::forward<T>(from);
}
#endif

template <class T>
void assign_any_impl(T& t, boost::any const& from) {
  t = boost::any_cast<T>(from);
}

struct assign_traits_exception : std::exception {
  std::string whatstr;
  assign_traits_exception(std::string const& what = "no assign / default construct")
      : whatstr("assign_traits: " + what) {}
  char const* what() const noexcept override { return whatstr.c_str(); }
};

#define NO_ASSIGN_MEMBER(Self)                                                                              \
  friend inline void assign_impl(Self& s, Self const&) {                                                    \
    throw graehl::assign_traits_exception(#Self " has no assignment operator (or friend assign_impl)");     \
  }                                                                                                         \
  friend inline void assign_any_impl(Self& s, boost::any const&) {                                          \
    throw graehl::assign_traits_exception(#Self " has no assignment operator (or friend assign_any_impl)"); \
  }

#define NO_INIT_MEMBER(Self)                                                                          \
  friend inline void init_impl(Self&) {                                                               \
    throw graehl::assign_traits_exception(#Self " has no default constructor (or friend init_impl)"); \
  }


#define NO_INIT_OR_ASSIGN_MEMBER(Self) NO_ASSIGN_MEMBER(Self) NO_INIT_MEMBER(Self)

template <class T>
void call_init(T& t) {
  init_impl(t);
}

template <class T>
void call_assign(T& t, T const& from) {
  assign_impl(t, from);
}

#if GRAEHL_CPP11
template <class T>
void call_assign(T& t, T&& from) {
  assign_impl(t, std::forward<T>(from));
}
#endif

template <class T>
void call_assign_any(T& t, boost::any const& from) {
  assign_any_impl(t, from);
}


struct no_assign {
  template <class T>
  static inline void init(T& t) {
    throw assign_traits_exception();
  }
  template <class T>
  static inline void assign(T& t, T const& from) {
    throw assign_traits_exception("no assign");
  }
  template <class T>
  static inline void assign_any(T& t, boost::any const& a) {
    throw assign_traits_exception("no assign");
  }
};

struct init_no_assign : no_assign {
  template <class T>
  static inline void init(T& t) {
    call_init(t);
  }
};

struct assignable {
  template <class T>
  static inline void init(T& t) {
    throw assign_traits_exception();
  }
#if GRAEHL_CPP11
  template <class T>
  static inline void call_assign_impl(T& t, T&& from) {
    call_assign(t, std::forward<T>(from));
  }
#else
  template <class T>
  static inline void assign(T& t, T const& from) {
    call_assign(t, from);
  }
#endif
  template <class T>
  static inline void assign_any(T& t, boost::any const& a) {
    call_assign_any(t, a);
  }
};

template <class T, class Enable = void>
struct assign_traits : assignable {
  static inline void init(T& t) { call_init(t); }
  static inline void assign(T& t, T const& from) { call_assign(t, from); }
#if GRAEHL_CPP11
  void call_assign_impl(T& t, T&& from) { call_assign(t, std::forward<T>(from)); }
#endif
  static inline void assign_any(T& t, boost::any const& a) { call_assign_any(t, a); }
};


}

#endif
