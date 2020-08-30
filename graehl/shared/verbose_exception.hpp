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

    throw exceptions holding a message, and function/file/line info.
*/

#ifndef GRAEHL_SHARED__VERBOSE_EXCEPTION_HPP
#define GRAEHL_SHARED__VERBOSE_EXCEPTION_HPP
#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace graehl {

// no line numbers etc. init w/ a string. if you want non-string, try THROW_MSG(type, a << b)
#define SIMPLE_EXCEPTION_PREFIX(ExceptionClass, msgPre)                     \
  struct ExceptionClass : public std::exception {                           \
    std::string message;                                                    \
    ExceptionClass(std::string const& m = "error") : message(msgPre + m) {} \
    ExceptionClass(ExceptionClass const& o) : message(o.message) {}         \
    char const* what() const noexcept override { return message.c_str(); }            \
  }

#define SIMPLE_EXCEPTION_CLASS(ExceptionClass) SIMPLE_EXCEPTION_PREFIX(ExceptionClass, #ExceptionClass ": ")

struct verbose_exception : public std::exception {
  char const* file;
  char const* function;
  unsigned line;
  std::string message;

  verbose_exception() : file(""), function(""), line(), message("unspecified verbose_exception") {}
  explicit verbose_exception(char const* m) : file(""), function(""), line(), message(m) {}
  explicit verbose_exception(std::string const& m) : file(""), function(""), line(), message(m) {}
  verbose_exception(verbose_exception const& o)
      : file(o.file), function(o.function), line(o.line), message(o.message) {}

  verbose_exception(char const* fun, char const* fil, unsigned lin) : file(fil), function(fun), line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "].";
    message = mbuf.str();
  }

  template <class M1>
  verbose_exception(char const* fun, char const* fil, unsigned lin, M1 const& m1)
      : file(fil), function(fun), line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ".";
    message = mbuf.str();
  }

  template <class M1, class M2>
  verbose_exception(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2)
      : file(fil), function(fun), line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ".";
    message = mbuf.str();
  }

  template <class M1, class M2, class M3>
  verbose_exception(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2, M3 const& m3)
      : file(fil), function(fun), line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ' ' << m3 << ".";
    message = mbuf.str();
  }

  template <class M1, class M2, class M3, class M4>
  verbose_exception(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2, M3 const& m3,
                    M4 const& m4)
      : file(fil), function(fun), line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ' ' << m3 << ' ' << m4 << ".";
    message = mbuf.str();
  }

  template <class M1, class M2, class M3, class M4, class M5>
  verbose_exception(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2, M3 const& m3,
                    M4 const& m4, M5 const& m5)
      : file(fil), function(fun), line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ' ' << m3 << ' ' << m4
         << ' ' << m5 << ".";
    message = mbuf.str();
  }

  char const* what() const noexcept override { return message.c_str(); } // NOLINT
};
}

#define VTHROW_A(type) throw type(__FUNCTION__, __FILE__, __LINE__)
#define VTHROW_A_1(type, arg) throw type(__FUNCTION__, __FILE__, __LINE__, arg)
#define VTHROW_A_2(type, arg, arg2) throw type(__FUNCTION__, __FILE__, __LINE__, arg, arg2)
#define VTHROW_A_3(type, arg, arg2, arg3) throw type(__FUNCTION__, __FILE__, __LINE__, arg, arg2, arg3)
#define VTHROW_A_4(type, arg, arg2, arg3, arg4) \
  throw type(__FUNCTION__, __FILE__, __LINE__, arg, arg2, arg3, arg4)

#define VTHROW VTHROW_A(graehl::verbose_exception)
#define VTHROW_1(a1) VTHROW_A_1(graehl::verbose_exception, a1)
#define VTHROW_2(a1, a2) VTHROW_A_2(graehl::verbose_exception, a1, a2)
#define VTHROW_3(a1, a2, a3) VTHROW_A_3(graehl::verbose_exception, a1, a2, a3)
#define VTHROW_4(a1, a2, a3, a4) VTHROW_A_4(graehl::verbose_exception, a1, a2, a3, a4)

#define THROW_MSG(type, msg) \
  do {                       \
    std::stringstream out;   \
    out << msg;              \
    throw type(out.str());   \
  } while (0)
#define VTHROW_A_MSG(type, msg)                              \
  do {                                                       \
    std::stringstream out;                                   \
    out << msg;                                              \
    throw type(__FUNCTION__, __FILE__, __LINE__, out.str()); \
  } while (0)
#define VTHROW_MSG(msg) VTHROW_A_MSG(graehl::verbose_exception, msg)

#define VERBOSE_EXCEPTION_WRAP(etype)                                                                           \
  explicit etype(std::string const& msg = "error") : graehl::verbose_exception("(" #etype ") " + msg) {}                 \
  etype(char const* fun, char const* fil, unsigned lin)                                                         \
      : graehl::verbose_exception(fun, fil, lin, "(" #etype ") ") {}                                            \
  template <class M1>                                                                                           \
  etype(char const* fun, char const* fil, unsigned lin, M1 const& m1)                                           \
      : graehl::verbose_exception(fun, fil, lin, "(" #etype ") ", m1) {}                                        \
  template <class M1, class M2>                                                                                 \
  etype(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2)                             \
      : graehl::verbose_exception(fun, fil, lin, "(" #etype ") ", m1, m2) {}                                    \
  template <class M1, class M2, class M3>                                                                       \
  etype(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2, M3 const& m3)               \
      : graehl::verbose_exception(fun, fil, lin, "(" #etype ") ", m1, m2, m3) {}                                \
  template <class M1, class M2, class M3, class M4>                                                             \
  etype(char const* fun, char const* fil, unsigned lin, M1 const& m1, M2 const& m2, M3 const& m3, M4 const& m4) \
      : graehl::verbose_exception(fun, fil, lin, "(" #etype ") ", m1, m2, m3, m4) {}

#define VERBOSE_EXCEPTION_DECLARE(etype)     \
  struct etype : graehl::verbose_exception { \
    VERBOSE_EXCEPTION_WRAP(etype)            \
  };


#endif
