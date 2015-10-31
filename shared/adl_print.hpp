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

     several levels of indirection to ensure we can find the free 'o << val' or
     'print(o, val, state)' member in the same namespace as val (or o or state)

     the idea is that impls of these can also be free to call adl::adl_print, or
     if they know what they're doing, the ultimate x << val or print(o, val,
     state) directly.

     further, this gives us a chance to directly print to things besides
     ostreams (e.g. string_buffer, which should be more direct/performant than
     stringstream)

     finally, helper o << printer(val, state) calls print(o, val, state)

     and o << printer(val) forces adl::adl_print(o, v) - which can locate
     defaults for pair, collection, etc.

     note that (pre-c++11) printer objects capture a reference to val and so
     (would have to double check standard) like std::max can't be safely passed
     a computed temporary value. presumably the AdlPrinterMove and PrinterMove
     classes will be selected for temporary values, which is definitely safe (if
     a move ctor exists).
*/

#ifndef ADL_PRINT_GRAEHL_2015_10_29_HPP
#define ADL_PRINT_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <utility>
#include <iostream>
#include <sstream>
#include <graehl/shared/type_traits.hpp>
#include <graehl/shared/is_container.hpp>

#ifndef GRAEHL_ADL_PRINTER_MOVE_OVERLOAD
/** (C++11 only): the AdlPrinterMove and PrinterMove classes will be selected
     for temporary values, which is definitely safe (if a move ctor exists).
*/
#define GRAEHL_ADL_PRINTER_MOVE_OVERLOAD 1
#endif

/// this namespace will contain no user types so should be the last resort. but
/// that's not how ADL works (priority ignores namespace, using, ADL), so
/// they're made worse matches (lower priority) if possible.
namespace adl_default {

/// careful to avoid noncontainers (ADL doesn't help w/ ambiguity)
template <class O, class V, class If = typename graehl::enable_if<graehl::is_nonstring_container<V>::value>::type>
O& operator<<(O& o, V const& v);

#if GRAEHL_ADL_PRINT_CONTAINER3
template <class O, class V, class S,
          class If = typename graehl::enable_if<graehl::is_nonstring_container<V>::value>::type>
void print(O& o, V const& v, S const& s);
#endif

template <class O, class A, class B>
void operator<<(O&, std::pair<A, B> const&);
template <class O, class A, class B, class S>
void print(O&, std::pair<A, B> const&, S const&);

// TODO: alternatively: make state-sequence-print decay to no state after 1 go (i.e. have a depth template
// arg)?
// TODO: is_fundamental?
template <class O, class V, class S>
typename graehl::enable_if<!graehl::is_class<V>::value>::type print(O& o, V const& v, S const&) {
  o << v;
}
}

/// several levels of indirection to ensure we can find the free 'o << val' or
/// 'print(o, val, state)' member in the same namespace as val (or o or state)
namespace adl {

template <class O, class V>
inline void adl_print(O& o, V const& val);
template <class O, class V, class S>
void adl_print(O& o, V const& val, S const& s);

template <class V, class Enable = void>
struct Print {
  template <class O>
  static void call(O& o, V const& v) {
    using namespace adl_default;
    o << v;
  }
  template <class O, class S>
  static void call(O& o, V const& v, S const& s) {
    using namespace adl_default;
    print(o, v, s);
  }

// TODO: maybe. need to test in C++98, msvc. for now use adl_to_string.hpp
#if 0
  static std::string str(V const& v) {
#if 0
    std::string r;
    call(r, v);
    return r;
#else
    std::stringstream r;
    call((std::ostream&)r, v);
    return r.str();
#endif
  }
  template <class S>
  static std::string str(V const& v, S& s) {
    std::string r;
    call(r, v, s);
    return r;
  }
#endif
};

template <>
struct Print<std::string, void> {
  typedef std::string V;
  template <class O>
  static void call(O& o, V const& v) {
    o << v;
  }
  template <class O, class S>
  static void call(O& o, V const& v, S const& s) {
    using namespace adl_default;
    print(o, v, s);
  }
  static std::string const& str(V const& v) { return v; }
#if 0
  template <class S>
  static std::string str(V const& v, S& s) {
    std::string r;
    call(r, v, s);
    return r;
  }
#endif
};

template <class O, class V>
inline void adl_print(O& o, V const& v) {
  Print<V>::call(o, v);
}
template <class O, class V, class S>
void adl_print(O& o, V const& v, S const& s) {
  Print<V>::call(o, v, s);
}


/// 0 char means no output
template <char Space = ' ', char Open = '[', char Close = ']'>
struct list_format {
  bool first;
  list_format() : first(true) {}

  typedef Print<char> CharPrint;
  template <class O>
  void open(O& o) {
    if (Open) CharPrint::call(o, Open);
  }
  template <class O>
  void close(O& o) {
    if (Close) CharPrint::call(o, Close);
  }
  template <class O>
  void space(O& o) {
    if (first)
      first = false;
    else
      CharPrint::call(o, Open);
  }
  template <class O, class V>
  void element(O& o, V const& v) {
    space(o);
    adl::Print<V>::call(o, v);
  }
  template <class O, class V, class S>
  void element(O& o, V const& v, S const& s) {
    space(o);
    adl::Print<V>::call(o, v, s);
  }
};
}

namespace adl_default {
template <class O, class V, class If>
O& operator<<(O& o, V const& v) {
  adl::list_format<> format;
  format.open(o);
  for (typename V::const_iterator i = v.begin(), e = v.end(); i != e; ++i) format.element(o, *i);
  format.close(o);
  return o;
}

#if GRAEHL_ADL_PRINT_CONTAINER3
template <class O, class V, class S, class If>
void print(O& o, V const& v, S const& s) {
  adl::list_format<> format;
  format.open(o);
  for (typename V::const_iterator i = v.begin(), e = v.end(); i != e; ++i) format.element(o, v, s);
  format.close(o);
}
#endif

template <class O, class A, class B>
void operator<<(O& o, std::pair<A, B> const& v) {
  adl::adl_print(o, v.first);
  o << '=';
  adl::adl_print(o, v.second);
}
template <class O, class A, class B, class S>
void print(O& o, std::pair<A, B> const& v, S const& s) {
  adl::adl_print(o, v.first, s);
  o << '=';
  adl::adl_print(o, v.second, s);
}
}


namespace graehl {

/**
   S should be a ref or pointer or lightweight WARNING: v and s are both
   potentially captured by reference, so don't pass a temporary (same issue as
   std::max). should be safe in context of immediately printing rather than
   holding forever
*/
template <class V, class S>
struct Printer {
  V const& v;
  S const& s;
  Printer(V const& v, S const& s) : v(v), s(s) {}
};

/// it's important to return ostream and not the more specific stream.
template <class V, class S>
inline std::ostream& operator<<(std::ostream& out, Printer<V, S> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V, class S>
typename enable_if<is_container<O>::value, O>::type& operator<<(O& out, Printer<V, S> const& x) {
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

template <class V, class S>
Printer<V, S> printer(V const& v, S const& s) {
  return Printer<V, S>(v, s);
}

template <class V>
struct AdlPrinter {
  V v;
  AdlPrinter(V v) : v(v) {}
};

/// it's important to return ostream and not the more specific stream.
template <class V>
inline std::ostream& operator<<(std::ostream& out, AdlPrinter<V> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V>
typename enable_if<is_container<O>::value, O>::type& operator<<(O& out, AdlPrinter<V> const& x) {
  ::adl::Print<V>::call(out, x.v);
  return out;
}

#if __cplusplus >= 201103L && GRAEHL_ADL_PRINTER_MOVE_OVERLOAD
template <class V, class S>
struct PrinterMove {
  V v;
  S s;
  PrinterMove(V&& v, S s) : v(std::forward<V>(v)), s(s) {}
};

/// it's important to return ostream and not the more specific stream.
template <class V, class S>
inline std::ostream& operator<<(std::ostream& out, PrinterMove<V, S> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V, class S>
typename enable_if<is_container<O>::value, O>::type& operator<<(O& out, PrinterMove<V, S> const& x) {
  ::adl::Print<V>::call(out, x.v, x.s);
  return out;
}

template <class V>
struct AdlPrinterMove {
  V v;
  AdlPrinterMove(V&& v) : v(std::forward<V>(v)) {}
};


/// it's important to return ostream and not the more specific stream.
template <class V>
inline std::ostream& operator<<(std::ostream& out, AdlPrinterMove<V> const& x) {
  // must be found by ADL - note: typedefs won't help.
  // that is, if you have a typedef and a shared_ptr, you have to put your print in either ns LW or boost
  ::adl::Print<V>::call(out, x.v);
  return out;
}

/// for O = e.g. string_builder
template <class O, class V>
typename enable_if<is_container<O>::value, O>::type& operator<<(O& out, AdlPrinterMove<V> const& x) {
  ::adl::Print<V>::call(out, x.v);
  return out;
}

#if 0
// is_lvalue_reference
template <class V>
AdlPrinter<V const&> printer_impl(V&& v, true_type) {
  return AdlPrinter<V const&>(v);
}

template <class V>
AdlPrinterMove<V> printer_impl(V&& v, false_type) {
  return AdlPrinterMove<V>(std::forward<V>(v));
}

// is_lvalue_reference
template <class V, class S>
Printer<V const&, S> printer_impl(V&& v, S const& s, true_type) {
  return Printer<V const&, S>(v, s);
}

template <class V, class S>
PrinterMove<V, S> printer_impl(V&& v, S&& s, false_type) {
  return PrinterMove<V, S>(std::forward<V>(v), s);
}

template <class V>
auto printer(V&& v) -> decltype(printer_impl(std::forward<V>(v), is_lvalue_reference<V>())) {
  return printer_impl(std::forward<V>(v), is_lvalue_reference<V>());
}

template <class V, class S>
auto printer(V&& v, S const& s) -> decltype(printer_impl(std::forward<V>(v), s, is_lvalue_reference<V>())) {
  return printer_impl(std::forward<V>(v), s, is_lvalue_reference<V>());
}

template <class V>
AdlPrinterMove<V> printer(V&& v) {
  return AdlPrinterMove<V>(std::forward<V>(v));
}

#else

template <class V, class Lvalue = true_type>
struct AdlPrinterType {
  typedef AdlPrinter<V const&> type;
};

template <class V>
struct AdlPrinterType<V, false_type> {
  typedef AdlPrinterMove<V> type;
};

template <class V, class S, class Lvalue = true_type>
struct PrinterType {
  typedef Printer<V const&, S> type;
};

template <class V, class S>
struct PrinterType<V, S, false_type> {
  typedef PrinterMove<V, S> type;
};

template <class V>
typename AdlPrinterType<V, is_lvalue_reference<V> >::type printer(V&& v) {
  return std::forward<V>(v);
}

template <class V, class S>
typename PrinterType<V, S, is_lvalue_reference<V> >::type printer(V&& v, S const& s) {
  return typename PrinterType<V, S, is_lvalue_reference<V> >::type(std::forward<V>(v), s);
}

#endif

#else
/// warning: captures reference.
template <class V>
AdlPrinter<V> printer(V const& v) {
  return v;
}
#endif


}

#endif
