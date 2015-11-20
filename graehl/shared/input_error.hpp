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
/**
   error message showing error position in input file (using seek if possible)
*/

#ifndef GRAEHL__SHARED__INPUT_ERROR_HPP
#define GRAEHL__SHARED__INPUT_ERROR_HPP
#pragma once

#ifndef INPUT_ERROR_TELLG
#define INPUT_ERROR_TELLG 1
#endif

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

namespace graehl {

#ifndef GRAEHL__ERROR_CONTEXT_CHARS
#define GRAEHL__ERROR_CONTEXT_CHARS 50
#endif
#ifndef GRAEHL__ERROR_PRETEXT_CHARS
#define GRAEHL__ERROR_PRETEXT_CHARS 20
#endif

inline void throw_parse_error(std::string const& context, std::string const& complaint = "parse error",
                              std::string::size_type pos = std::string::npos) {
  using namespace std;
  ostringstream o;
  o << complaint << ": ";
  if (pos == string::npos)
    o << context;
  else
    o << string(context, 0, pos) << "<ERROR HERE>" << string(context, pos);
  throw runtime_error(o.str());
}

template <class C>
inline C scrunch_char(C c, char with = '/') {
  switch (c) {
    case '\n':
    case '\t':
      return with;
    default:
      return c;
  }
}

template <class O, class C>
inline void output_n(O& o, C const& c, unsigned n) {
  for (unsigned i = 0; i < n; ++i) o << c;
}

template <class Ic, class It, class Oc, class Ot>
inline std::streamoff show_error_context(std::basic_istream<Ic, It>& in, std::basic_ostream<Oc, Ot>& out,
                                         unsigned prechars = GRAEHL__ERROR_PRETEXT_CHARS,
                                         unsigned postchars = GRAEHL__ERROR_CONTEXT_CHARS) {
  char c;
  std::streamoff actual_pretext_chars = 0;
  // if (fstrm * fs = dynamic_cast<fstrm *>(&in)) { // try tell/seek always, -1 return if it fails anyway
  bool ineof = in.eof();
  in.clear();
  std::streamoff before = in.tellg();
  in.unget();
  in.clear();
  // DBP(before);
  if (before >= 0) {
    in.seekg(-(int)prechars, std::ios_base::cur);
    std::streamoff after(in.tellg());
    if (!in) {
      in.clear();
      in.seekg(after = 0);
    }
    actual_pretext_chars = before - after;
  } else
    actual_pretext_chars = 1;  // from unget
  in.clear();
  out << "INPUT ERROR: ";
  if (before >= 0) out << " reading byte #" << before + 1;
  if (ineof) out << " (at EOF)";
  out << " (^ marks the read position):\n";
  const unsigned indent = 3;
  output_n(out, '.', indent);
  unsigned ip, ip_lastline = 0;
  for (ip = 0; ip < actual_pretext_chars; ++ip) {
    if (in.get(c)) {
      ++ip_lastline;
#if 1
      out << scrunch_char(c);
#else
      // show newlines
      out << c;
      if (c == '\n') {
        ip_lastline = 0;
      }
#endif
    } else
      break;
  }

  if (ip != actual_pretext_chars)
    out << "<<<WARNING: COULD NOT READ " << prechars << " pre-error characters (only got " << ip << ")>>>";
  for (unsigned i = 0; i < GRAEHL__ERROR_CONTEXT_CHARS; ++i)
    if (in.get(c))
      out << scrunch_char(c);
    else
      break;

  out << '\n';
  output_n(out, ' ', indent);
  for (unsigned i = 0; i < ip_lastline; ++i) out << ' ';
  out << '^' << '\n';
  return before >= 0 ? before : -1;
}

template <class Exception, class Ic, class It>
void throw_input_exception(std::basic_istream<Ic, It>& in, std::string const& error = "",
                           const char* item = "input", std::size_t number = 0) {
  std::ostringstream err;
  err << "Error reading";
  if (item) err << ' ' << item << " # " << number;
  err << ": " << error << '\n';
  std::streamoff where = show_error_context(in, err);
#if INPUT_ERROR_TELLG
  if (where >= 0) err << "(file position " << where << ")";
#endif
  err << '\n';
  throw Exception(err.str());
}

template <class Ic, class It>
void throw_input_error(std::basic_istream<Ic, It>& in, std::string const& error = "",
                       const char* item = "input", std::size_t number = 0) {
  throw_input_exception<std::runtime_error>(in, error, item, number);
}


}

#endif
