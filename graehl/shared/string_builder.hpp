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

  append formatted ints floats etc to a c++11 string or pre-c++11 vector<char> -
  since older std::string may have poor append performance.
*/

#ifndef STRING_BUILDER_GRAEHL_2015_10_29_HPP
#define STRING_BUILDER_GRAEHL_2015_10_29_HPP
#pragma once

#include <graehl/shared/string_to.hpp>
//
#include <graehl/shared/adl_to_string.hpp>
#include <graehl/shared/base64.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/string_buffer.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <string>
#include <vector>

namespace graehl {

struct string_builder : string_buffer {
  template <class Int>
  string_builder& base64LE_pad(Int x) {
    base64LE_append_pad(*this, x);
    return *this;
  }
  template <class Int>
  string_builder& base64LE(Int x) {
    base64LE_append(*this, x);
    return *this;
  }
  bool operator==(std::string const& str) {
    std::size_t const len = str.size();
    return len == size() && !std::memcmp(begin(), &*str.begin(), len);
  }

  typedef char const* const_iterator;

  typedef char* iterator;

#if GRAEHL_CPP11
  iterator begin() { return const_cast<iterator>(this->data()); }
  const_iterator begin() const { return this->data(); }
#else
  const_iterator data() const { return begin(); }
  iterator data() { return begin(); }
  iterator begin() { return &*string_buffer::begin(); }
  const_iterator begin() const { return &*string_buffer::begin(); }
#endif
#if _WIN32 && (!defined(_SECURE_SCL) || _SECURE_SCL)
  iterator end() { return empty() ? 0 : &*string_buffer::begin() + string_buffer::size(); }
  const_iterator end() const { return empty() ? 0 : &*string_buffer::begin() + string_buffer::size(); }
#else
  iterator end() { return const_cast<iterator>(&*string_buffer::end()); }
  const_iterator end() const { return &*string_buffer::end(); }
#endif

  std::pair<char const*, char const*> slice() const {
    return std::pair<char const*, char const*>(begin(), end());
  }


  /**
     for backtracking.
  */
  struct unappend {
    string_buffer& builder;
    std::size_t size;
    unappend(string_buffer& builder) : builder(builder), size(builder.size()) {}
    ~unappend() {
      assert(builder.size() >= size);
      builder.resize(size);
    }
  };

  string_builder& shrink(std::size_t oldsize) {
    assert(oldsize <= size());
    this->resize(oldsize);
    return *this;
  }

  template <class Val>
  string_builder& operator<<(Val const& val) {
    return (*this)(val);
  }

  string_builder() { this->reserve(80); }
  string_builder(unsigned reserveChars) { this->reserve(reserveChars); }
  explicit string_builder(std::string const& str) : string_buffer(str.begin(), str.end()) {}
  string_builder& clear() {
    string_buffer::clear();
    return *this;
  }
  template <class S>
  string_builder& append(S const& s) {
    return (*this)(s);
  }
  template <class S>
  string_builder& append(S const& s1, S const& s2) {
    return (*this)(s1, s2);
  }
  template <class S>
  string_builder& range(S const& s, word_spacer& sp) {
    return range(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range(S s1, S const& s2, word_spacer& sp) {
    for (; s1 != s2; ++s1) {
      sp.append(*this);
      (*this)(*s1);
    }
    return *this;
  }
  template <class S>
  string_builder& range(S const& s, char space = ' ') {
    word_spacer sp(space);
    return range(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range(S s1, S const& s2, char space = ' ') {
    word_spacer sp(space);
    return range(s1, s2, sp);
  }

  /**
     for printing simple numeric values - maxLen must be sufficient or else
     nothing is appended. also note that printf format strings need to match the
     Val type precisely - explicit cast or template arg may make this more
     obvious but compiler should know how to warn/error on mismatch
  */
  template <class Val>
  string_builder& nprintf(unsigned maxLen, char const* fmt, Val val) {
    std::size_t sz = this->size();
    this->resize(sz + maxLen);
    // For Windows, snprintf is provided by shared/sprintf.hpp (in global namespace)
    unsigned written = (unsigned)snprintf(begin() + sz, maxLen, fmt, val);
    if (written >= maxLen) written = 0;
    this->resize(sz + written);
    return *this;
  }
  template <class Val, class Val2>
  string_builder& nprintf(unsigned maxLen, char const* fmt, Val val, Val2 val2) {
    std::size_t sz = this->size();
    this->resize(sz + maxLen);
    unsigned written = (unsigned)snprintf(begin() + sz, maxLen, fmt, val, val2);
    if (written >= maxLen) written = 0;
    this->resize(sz + written);
    return *this;
  }
  /// for fmt = e.g. "%*d" "%*g" "%.*g" with int field width arg.
  template <class Val>
  string_builder& nprintf_width(char const* fmt, Val val, unsigned width, unsigned extrabuf = 8) {
    return nprintf(width + extrabuf, fmt, (int)width, val);
  }

  enum { kMaxInt32Chars = 12, kMaxInt64Chars = 22 };

  string_builder& operator()(unsigned char x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }
  string_builder& operator()(unsigned short x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }
  string_builder& operator()(unsigned x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }
  string_builder& operator()(std::size_t x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }

  string_builder& digits(double x, unsigned w = 8) { return nprintf_width(fmt_double_precision2, x, w); }

  /// float can distinguish between final 8 9 0 in all cases for the first 6 digits
  string_builder& significant(float x, unsigned w = 6) { return digits(x, w); }

  /// double can distinguish between final 8 9 0 in all cases for the first 15 digits
  string_builder& significant(double x, unsigned w = 15) { return digits(x, w); }

  /**
     enough digits that you get the same bits back when parsing the decimal text
  */
  string_builder& roundtrip(float x) { return nprintf(16, fmt_double_for_float_roundtrip, (double)x); }

  /**
     enough digits that you get the same bits back when parsing the decimal text
  */
  string_builder& roundtrip(double x) { return nprintf(32, fmt_double_roundtrip, x); }

  string_builder& operator()(char c) {
    this->push_back(c);
    return *this;
  }
  string_builder& operator()(string_builder const& o) {
    (*this)(o.begin(), o.end());
    return *this;
  }
  template <class CharIter>
  string_builder& operator()(CharIter i, CharIter end) {
    this->insert(string_buffer::end(), i, end);
    return *this;
  }
  string_builder& operator()(std::pair<char const*, char const*> word) {
    this->insert(string_buffer::end(), word.first, word.second);
    return *this;
  }
  string_builder& operator()(std::string const& s) {
    (*this)(s.begin(), s.end());
    return *this;
  }
  string_builder& operator()(char const* s) {
    for (; *s; ++s) this->push_back(*s);
    return *this;
  }
  string_builder& operator()(char const* s, unsigned len) { return (*this)(s, s + len); }

  template <class T>
  string_builder& operator()(T const& t) {
    adl::adl_append_to_string(*this, t);
    return *this;
  }
  string_builder& operator()(std::streambuf& ibuf) {
    typedef std::istreambuf_iterator<char> I;
    std::copy(I(&ibuf), I(), std::back_inserter(*this));
    return *this;
  }
  string_builder& operator()(std::istream& i) { return (*this)(*i.rdbuf()); }
  std::string& assign(std::string& str) {
    to(str);
    return str;
  }
#include <graehl/shared/warning_push.h>
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
// TODO: remove pragma
#if GRAEHL_CPP11
  explicit string_builder(char const* str) : string_buffer(str) {}
  /// destructive: move away from current buffer (can't use any more w/o clear)
  void to(std::string& str) { str = std::move((std::string&)*this); }
  std::string const& str() const { return *this; }
  std::string&& str() { return std::move(*this); }
  std::string* new_str() const { return new std::string(*this); }
  std::string const& strcopy() const { return *this; }
  string_builder& append(std::string const& str, std::string::size_type from,
                         std::string::size_type len = std::string::npos) {
    string_buffer::append(str, from, len);
    return *this;
  }
#else
  explicit string_builder(char const* str) : string_buffer(str, str + std::strlen(str)) {}
  void to(std::string& str) {
    str.assign(this->begin(), this->end());
    clear();
  }
  std::string str() {
    std::string r(this->begin(), this->end());
    clear();
    return r;
  }
  std::string const& strcopy() const { return std::string(this->begin(), this->end()); }
  std::string* new_str() const { return new std::string(this->begin(), this->end()); }
  string_builder& append(std::string const& str, std::string::size_type from,
                         std::string::size_type len = std::string::npos) {
    char const* data = str.data();
    return operator()(data + from, data + from + len);
  }
#endif
  string_builder& append_range(std::string const& str, std::string::size_type from, std::string::size_type to) {
    char const* data = str.data();
    assert(to >= from);
    return operator()(data + from, data + to);
  }
#include <graehl/shared/warning_pop.h>

  std::string skipPrefix(std::size_t prefixLen) const {
    return prefixLen > this->size() ? std::string() : std::string(this->begin() + prefixLen, this->end());
  }
  std::string str(std::size_t len) const {
    if (this->size() <= len) return *this;
    const_iterator b = this->begin();
    return std::string(b, b + len);
  }
  std::string shorten(std::size_t drop_suffix_chars) {
    std::size_t n = this->size();
    if (drop_suffix_chars > n) return std::string();
    const_iterator b = this->begin();
    return std::string(b, b + (n - drop_suffix_chars));
  }

  /**
     append space the second and subsequent times this is called with each
     initial 'bool first = true' (or every time if first == false)
  */
  string_builder& space_except_first(bool& first, char space = ' ') {
    if (!first) operator()(space);
    first = false;
    return *this;
  }

  template <class Sep>
  string_builder& sep_except_first(bool& first, Sep const& sep) {
    if (!first) operator()(sep);
    first = false;
    return *this;
  }

  string_builder& append_space(std::string const& space) {
    if (!this->empty()) operator()(space);
    return *this;
  }
  string_builder& append_space(char space = ' ') {
    if (!this->empty()) operator()(space);
    return *this;
  }

  string_builder& append_2space(char space = ' ') {
    if (!this->empty()) {
      operator()(space);
      operator()(space);
    }
    return *this;
  }

  string_builder& word(std::string const& t, std::string const& space) {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }
  string_builder& word(std::string const& t, char space = ' ') {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }

  string_builder& escape_char(char c) {
    char const quote = '"';
    char const backslash = '\\';
    if (c == quote || c == backslash) this->push_back(backslash);
    return (*this)(c);
  }

  template <class CharIter>
  string_builder& quoted(CharIter i, CharIter const& end) {
    char const quote = '"';
    this->push_back(quote);
    for (; i != end; ++i) escape_char(*i);
    this->push_back(quote);
    return *this;
  }

  template <class Chars>
  string_builder& quoted(Chars const& str) {
    return quoted(str.begin(), str.end());
  }

  template <class S>
  string_builder& range_quoted(S const& s, word_spacer& sp) {
    return range_quoted(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range_quoted(S s1, S const& s2, word_spacer& sp) {
    for (; s1 != s2; ++s1) {
      sp.append(*this);
      quoted(*s1);
    }
    return *this;
  }
  template <class S>
  string_builder& range_quoted(S const& s) {
    word_spacer sp;
    return range_quoted(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range_quoted(S s1, S const& s2) {
    word_spacer sp;
    return range_quoted(s1, s2, sp);
  }
  template <class Out>
  void print(Out& out) const {
    out.write(begin(), string_buffer::size());
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& out, string_builder const& self) {
    self.print(out);
    return out;
  }

  /// can't append anything else unless you first pop_back to remove the '\0'
  char* c_str() {
    this->push_back((char)0);
    return begin();
  }

  friend inline std::size_t hash_value(string_builder const& self) { return self.hash(); }
  std::size_t hash() const { return boost::hash_range(begin(), end()); }

  template <class Seq, class Sep>
  string_builder& join(Seq const& seq, Sep const& sep) {
    bool first = true;
    for (typename Seq::const_iterator i = seq.begin(), e = seq.end(); i != e; ++i)
      sep_except_first(first, sep)(*i);
    return *this;
  }
  template <class Seq, class Sep>
  string_builder& join(Seq const& seq) {
    return join(seq, ' ');
  }

  template <class Seq, class After, class Sep>
  string_builder& join_and(Seq const& seq, After const& after, Sep const& sep) {
    for (typename Seq::const_iterator i = seq.begin(), e = seq.end(); i != e; ++i) (*this) (*i)(sep);
    return (*this)(after);
  }

  std::string str_skip_first_char() const {
    char const* b = &*begin();
    std::size_t n = size();
    return std::string(n ? b + 1 : b, b + n);
  }
  std::pair<char const*, char const*> slice_skip_first_char() const {
    return std::pair<char const*, char const*>(begin() + !empty(), end());
  }
  template <class Out>
  void print_skip_first_char(Out& out) const {
    std::size_t sz = string_buffer::size();
    if (sz) out.write(begin() + 1, --sz);
  }
};

template <class Seq, class Sep>
std::string joined_seq(Seq const& seq, Sep const& sep) {
  string_builder b;
  b.join(seq, sep);
  return std::string(b.begin(), b.end());
}

template <class V>
inline std::string to_string_brackets(V const& val) {
  string_builder b;
  b('[');
  bool first = true;
  for (typename V::const_iterator i = val.begin(), e = val.end(); i != e; ++i) {
    if (first)
      first = false;
    else
      b(' ');
    b(*i);
  }
  b(']');
  return b.str();
}


}

#endif
