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

    optimized implementations of common line-input whitespace-removing operations.
*/

#ifndef GRAEHL__SHARED__STRING_MATCH_HPP
#define GRAEHL__SHARED__STRING_MATCH_HPP
#pragma once

#include <graehl/shared/function_macro.hpp>
#include <graehl/shared/null_terminated.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/push_backer.hpp>

#include <string>
#include <iterator>
#include <stdexcept>
#include <locale>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <cctype>
#endif

namespace {  // anon
static std::string ascii_whitespace = "\n\r\t ";
}

namespace graehl {

template <typename charT>
struct ascii_case_insensitive_equal {
  ascii_case_insensitive_equal(const std::locale& loc) : loc_(loc) {}
  bool operator()(charT ch1, charT ch2) { return std::toupper(ch1, loc_) == std::toupper(ch2, loc_); }

 private:
  const std::locale& loc_;
};

// find substring start index or -1 if not found (case insensitive)
template <typename T>
typename T::size_type ci_find_substr(const T& str, const T& substr, const std::locale& loc = std::locale()) {
  typename T::const_iterator it = std::search(str.begin(), str.end(), substr.begin(), substr.end(),
                                              ascii_case_insensitive_equal<typename T::value_type>(loc));
  return it == str.end() ? -1 : it - str.begin();
}

/// return new end() that would remove trailing '\r' '\n' or '\r\n', if any
template <class S>
typename S::const_iterator chomp_end(S const& s) {
  if (s.empty()) return s.end();
  typename S::const_iterator b = s.begin(), i = s.end() - 1;
  if (i == b) return *i == '\n' || *i == '\r' ? i : ++i;
  if (*i == '\n') --i;
  if (*i == '\r') --i;
  return ++i;
}

/// return w/ removed trailing '\r' '\n' or '\r\n', if any
template <class S>
S chomp(S const& s) {
  if (s.empty()) return s;
  typename S::const_iterator b = s.begin(), i = s.end() - 1;
  if (i == b) return *i == '\n' || *i == '\r' ? S() : s;
  if (*i == '\n') --i;
  if (*i == '\r') --i;
  return S(b, ++i);
}

template <class S>
S rtrim(S const& s, std::string const& ws = ascii_whitespace) {
  typename S::size_type e;
  e = s.find_last_not_of(ws);
  //    if (e==S::npos) return S(); //npos is -1, so -1+1 = 0
  assert(e != S::npos || e + 1 == 0);
  return S(s.begin(), s.begin() + e + 1);
}

template <class S>
S ltrim(S const& s, std::string const& ws = ascii_whitespace) {
  //    std::string ws="\n\t ";
  typename S::size_type b;
  b = s.find_first_not_of(ws);
  if (b == S::npos) return S();
  return S(s.begin() + b, s.end());
}

template <class S>
S trim(S const& s, std::string const& ws = ascii_whitespace) {
  //    std::string ws="\n\t ";
  typename S::size_type b, e;
  b = s.find_first_not_of(ws);
  if (b == S::npos) return S();
  e = s.find_last_not_of(ws);
  assert(e != S::npos);
  return S(s.begin() + b, s.begin() + e + 1);
}

template <class I>
std::string get_string_terminated(I& i, char terminator = '\n') {
  std::stringstream s;
  char c;
  while (i.get(c)) {
    if (c == terminator) return s.str();
    s.put(c);
  }
  throw std::runtime_error("EOF on input before string terminated");
}


template <class Str>
inline void erase_begin(Str& s, unsigned n) {
  s.erase(0, n);
}

template <class Str>
inline void erase_end(Str& s, unsigned n) {
  s.erase(s.length() - n, n);
}

template <class Str>
typename Str::size_type replace(Str& in, Str const& oldsub, Str const& newsub, typename Str::size_type pos = 0) {
  pos = in.find(oldsub, pos);
  if (pos == Str::npos) return pos;
  in.replace(pos, oldsub.length(), newsub);
  return pos + newsub.length();
}

// returns true if one was replaced
template <class Str>
bool replace_one(Str& in, Str const& oldsub, Str const& newsub, typename Str::size_type pos = 0) {
  return replace(in, oldsub, newsub, pos) != Str::npos;
}

// returns number of types we replaced
template <class Str>
unsigned replace_all(Str& in, Str const& oldsub, Str const& newsub, typename Str::size_type pos = 0) {
  unsigned n = 0;
  while ((pos = replace(in, oldsub, newsub, pos)) != Str::npos) ++n;
  return n;
}


template <class Str, class Sub>
bool contains_substring(Str const& str, Sub const& sub, typename Str::size_type pos = 0) {
  return str.find(sub, pos) != Str::npos;
}


// returns true and writes pos, n for substring between left-right brackets.  or false if brackets not found.
template <class Str>
inline bool substring_inside_pos_n(const Str& s, const Str& leftbracket, const Str& rightbracket,
                                   typename Str::size_type& pos, typename Str::size_type& n,
                                   typename Str::size_type start_from = 0) {
  typename Str::size_type rightpos;
  if (Str::npos == (pos = s.find(leftbracket, start_from))) return false;
  pos += leftbracket.length();
  if (Str::npos == (rightpos = s.find(rightbracket, pos))) return false;
  n = rightpos - pos;
  return true;
}

// first is first substring (left->right) between leftbracket and rightbracket in s.
// second is true if found, false if none found
template <class Str>
inline std::pair<Str, bool> substring_inside(const Str& s, const Str& leftbracket, const Str& rightbracket,
                                             typename Str::size_type start_from = 0) {
  typedef std::pair<Str, bool> Ret;
  typename Str::size_type pos, n;
  if (substring_inside_pos_n(s, leftbracket, rightbracket, pos, n, start_from))
    return Ret(Str(s, pos, n), true);
  else
    return Ret(Str(), false);
}

// parse both streams as a sequence of ParseAs, comparing for equality
template <class ParseAs, class Istream>
inline bool equal_streams_as_seq(Istream& i1, Istream& i2) {
  /* could almost write as istream_iterator<ParseAs>, std::equal - except that
   doesn't check both iterators' end
  */
  ParseAs v1, v2;
  for (;;) {
    bool got1 = (bool)(i1 >> v1);
    bool got2 = (bool)(i2 >> v2);
    if (got1) {
      if (!got2) return false;  // 2 ended first
    } else {
      if (!got2) return true;  // both ended together
      return false;  // 1 ended first
    }
    if (!(v1 == v2)) return false;  // value mismatch
  }
  // unreachable!
  assert(0);
}

template <class ParseAs, class Ch, class Tr>
inline bool equal_strings_as_seq(const std::basic_string<Ch, Tr>& s1, const std::basic_string<Ch, Tr>& s2) {
  std::basic_stringstream<Ch, Tr> i1(s1), i2(s2);
  return equal_streams_as_seq<ParseAs>(i1, i2);
}

// std::equal can only be called if sequences are same length!
template <class I1, class I2, class Equal>
inline bool equal_safe(I1 b1, I1 e1, I2 b2, I2 e2, Equal eq) {
  while (b1 != e1) {
    if (b2 == e2) return false;
    if (*b2++ != *e2++) return false;
  }
  // now b1 == e1
  return b2 == e2;
}

template <class I1, class I2>
inline bool equal_safe(I1 b1, I1 e1, I2 b2, I2 e2) {
  return equal_safe(b1, e1, b2, e2, equal_typeless());
}

// oops: didn't notice that I'd already implemented this before starts_with.  two implementations for testing
// anyway ;)
template <class Istr, class Isubstr>
inline bool match_begin(Istr bstr, Istr estr, Isubstr bsub, Isubstr esub) {
  while (bsub != esub) {
    if (bstr == estr) return false;
    if (*bsub++ != *bstr++) return false;
  }
  return true;
}

template <class Istr, class Prefix>
inline bool match_begin(Istr bstr, Istr estr, Prefix const& prefix) {
  return match_begin(bstr, estr, prefix.begin(), prefix.end());
}

template <class Str, class Prefix>
inline bool match_begin(Str str, Prefix const& prefix) {
  return match_begin(str.begin(), str.end(), prefix.begin(), prefix.end());
}

template <class Istr, class Isubstr>
inline bool match_end(Istr bstr, Istr estr, Isubstr bsub, Isubstr esub) {
  while (bsub != esub) {
    if (bstr == estr) return false;
    if (*--esub != *--estr) return false;
  }
  return true;
}

template <class Istr, class Suffix>
inline bool match_end(Istr bstr, Istr estr, Suffix const& suffix) {
  return match_end(bstr, estr, suffix.begin(), suffix.end());
}

template <class Str, class Suffix>
inline bool match_end(Str const& str, Suffix const& suffix) {
  return match_end(str.begin(), str.end(), suffix.begin(), suffix.end());
}

template <class Str, class Suffix>
inline bool match_end(Str const& str, char const* suffix) {
  return match_end(str, std::string(suffix));
}

template <class It1, class It2, class Pred>
inline bool starts_with(It1 str, It1 str_end, It2 prefix, It2 prefix_end, Pred equals) {
  for (;;) {
    if (prefix == prefix_end) return true;
    if (str == str_end) return false;
    if (!equals(*prefix, *str)) return false;
    ++prefix;
    ++str;
  }
  // unreachable
  assert(0);
}

template <class It1, class It2>
inline bool starts_with(It1 str, It1 str_end, It2 prefix, It2 prefix_end) {
  return starts_with(str, str_end, prefix, prefix_end, equal_typeless());
}

/*
//FIXME: provide skip-first-whitespace or skip-no-whitespace iterators.
template <class Ch, class Tr, class CharIt> inline
bool expect_consuming(std::basic_istream<Ch, Tr> &i, CharIt begin, CharIt end)
{
    typedef std::istream_iterator<Ch> II;
    II ibeg(i), iend;
    return match_begin(ibeg, iend, begin, end);
}
*/

template <class Ch, class Tr, class CharIt>
inline bool expect_consuming(std::basic_istream<Ch, Tr>& i, CharIt begin, CharIt end,
                             bool skip_first_ws = true) {
  if (begin == end) return true;
  Ch c;
  if (skip_first_ws)
    i >> c;
  else
    i.get(c);
  if (!i) return false;
  while (begin != end) {
    if (!i.get(c)) return false;
    if (c != *begin) return false;
  }
  return true;
  /* //NOTE: whitespace will be ignored!  so don't include space in expectation ...
      typedef std::istream_iterator<Ch> II;
      II ibeg(i), iend;
      return match_begin(ibeg, iend, begin, end);
  */
}

template <class Ch, class Tr, class Str>
inline bool expect_consuming(std::basic_istream<Ch, Tr>& i, const Str& str, bool skip_first_ws = true) {
  return expect_consuming(i, str.begin(), str.end(), skip_first_ws);
}


template <class Str>
inline bool starts_with(const Str& str, const Str& prefix) {
  return starts_with(str.begin(), str.end(), prefix.begin(), prefix.end());
}

template <class Str>
inline bool ends_with(const Str& str, const Str& suffix) {
  //        return starts_with(str.rbegin(), str.rend(), suffix.rbegin(), suffix.rend());
  return match_end(str.begin(), str.end(), suffix.begin(), suffix.end());
}

template <class Str, class StrSuffix>
inline bool strip_suffix(Str& str, const StrSuffix& suffix) {
  if (match_end(str.begin(), str.end(), suffix.begin(), suffix.end())) {
    str.erase(str.end() - suffix.size(), str.end());
    return true;
  } else
    return false;
}

template <class Str>
inline bool starts_with(const Str& str, char const* prefix) {
  return starts_with(str.begin(), str.end(), cstr_const_iterator(prefix), cstr_const_iterator());
  //    return starts_with(str, std::string(prefix));
}

template <class Istr, class Prefix>
inline bool starts_with(Istr bstr, Istr estr, Prefix prefix) {
  return starts_with(bstr, estr, prefix.begin(), prefix.end());
}

template <class Str>
inline bool ends_with(const Str& str, char const* suffix) {
  //  return match_end(str.begin(), str.end(), null_terminated_rbegin(suffix), null_terminated_rend(suffix));
  return ends_with(str, std::string(suffix));
}

template <class Istr, class Suffix>
inline bool ends_with(Istr bstr, Istr estr, Suffix suffix) {
  return match_end(bstr, estr, suffix.begin(), suffix.end());
}

// func(const Func::argument_type &val) - assumes val can be parsed from string tokenization (no whitespace)
template <class In, class Func>
inline void parse_until(const std::string& term, In& in, Func func) {
  std::string s;
  bool last = false;
  while (!last && (in >> s)) {
    if (!term.empty() && ends_with(s, term)) {
      last = true;
      erase_end(s, term.length());
    }
    if (s.empty()) break;
    typename Func::argument_type val;
    string_to(s, val);
    func(val);
  }
}

template <class In, class Cont>
inline void push_back_until(const std::string& term, In& in, Cont& cont) {
  parse_until(term, in, make_push_backer(cont));
}

template <class F>
inline void tokenize_key_val_pairs(const std::string& s, F& f, char pair_sep = ',', char key_val_sep = ':') {
  typedef typename F::key_type Key;
  typedef typename F::data_type Data;
  using namespace std;
  typedef pair<Key, Data> Component;
  typedef string::const_iterator It;
  Component to_add;
  for (It i = s.begin(), e = s.end();;) {
    for (It key_beg = i;; ++i) {
      if (i == e) return;
      if (*i == key_val_sep) {  // [last, i) is key
        string_to(string(key_beg, i), to_add.first);
        break;  // done key, expect val
      }
    }
    for (It val_beg = ++i;; ++i) {
      if (i == e || *i == pair_sep) {
        string_to(string(val_beg, i), to_add.second);
        f(to_add);
        if (i == e) return;
        ++i;
        break;  // next key/val
      }
    }
  }
}


/**
   return whether ' ' is found in [s.begin()+advance, s.begin()+at_most). updates
   advance to <= at_most but > advance, with advance pointing at the rightmost
   possible ' ' if any. note: requires at_most<s.size()
*/
inline bool indent_split_left(std::string const& s, std::string::size_type& advance,
                              std::string::size_type at_most) {
  assert(at_most < s.size());
  std::string::size_type at = s.rfind(' ', at_most);  // TODO: rfind bounded on more_than?
  if (at == std::string::npos || at <= advance) {
    advance = at_most;
    return false;
  }
  advance = at;
  assert(s[at] == ' ');
  return true;
}

/**
   return s.size() (ending column after newline).
*/

template <class Ostream>
unsigned newline_out(Ostream& out, std::string const& s) {
  out << '\n' << s;
  return (unsigned)s.size();
}

/**
   print s (splitting words on 1 or more ' ' including opening and closing ' ',
   with word wrap on or before indent_column. returns ending column, starting at
   at_column. continuation lines when spaces aren't found.
*/
template <class Ostream>
unsigned print_indent(Ostream& out, std::string const& s, unsigned at_column, unsigned max_column,
                      std::string const& nl_indent_str = "    ",
                      std::string const& continue_indent_str = " ...") {
  using std::string;
  typedef string::size_type strpos;
  strpos i = 0, end = s.find_last_not_of(' ');
  if (end == std::string::npos) return at_column;
  ++end;
  string::const_iterator sbeg = s.begin();
  while (i < end) {
    i = s.find_first_not_of(' ', i);  // skip space(s)
    if (i == string::npos) break;
    strpos remain = max_column - at_column;
    strpos from = i, upto = i + remain;
    if (upto >= end) {
      out << string(sbeg + from, sbeg + end);
      at_column += (unsigned)(end - from);
      break;
    } else {
      if (!remain) {
        at_column = newline_out(out, nl_indent_str);
        continue;
      }
      if (indent_split_left(s, i, upto)) {  // advances i
        // to a found space
        out << string(sbeg + from, sbeg + i);
        at_column = newline_out(out, nl_indent_str);
        ++i;  // move past the found space
      } else {
        out << string(sbeg + from, sbeg + i);
        at_column = newline_out(out, continue_indent_str);
      }
    }
    assert(i > from);
  }
  return at_column;
}


#ifdef GRAEHL_TEST
const char* TEST_starts_with[] = {"s", "st", "str", "str1"};

const char* TEST_ends_with[] = {"1", "r1", "tr1", "str1"};
// NOTE: could use substring but that's more bug-prone ;D

BOOST_AUTO_TEST_CASE(TEST_STRING_MATCH) {
  using namespace std;
  string s1("str1"), emptystr;
  BOOST_CHECK(starts_with(s1, emptystr));
  BOOST_CHECK(starts_with(emptystr, emptystr));
  BOOST_CHECK(ends_with(s1, emptystr));
  BOOST_CHECK(ends_with(emptystr, emptystr));
  BOOST_CHECK(!starts_with(s1, "str11"));
  BOOST_CHECK(!ends_with(s1, string("sstr1")));
  BOOST_CHECK(!ends_with(s1, "sstr1"));
  BOOST_CHECK(!starts_with(s1, string("str*")));
  BOOST_CHECK(!ends_with(s1, string("*tr1")));
  BOOST_CHECK(!ends_with(s1, string("str*")));
  BOOST_CHECK(!starts_with(s1, string("*tr1")));
  for (unsigned i = 0; i < 4; ++i) {
    string starts(TEST_starts_with[i]), ends(TEST_ends_with[i]);
    BOOST_CHECK(starts_with(s1, starts));
    BOOST_CHECK(starts_with(s1, starts.c_str()));
    BOOST_CHECK(ends_with(s1, ends));
    BOOST_CHECK(ends_with(s1, ends.c_str()));
    BOOST_CHECK(match_begin(s1.begin(), s1.end(), starts.begin(), starts.end()));
    BOOST_CHECK(match_end(s1.begin(), s1.end(), ends.begin(), ends.end()));
    if (i != 3) {
      BOOST_CHECK(!starts_with(s1, ends));
      BOOST_CHECK(!ends_with(s1, starts));
      BOOST_CHECK(!match_end(s1.begin(), s1.end(), starts.begin(), starts.end()));
      BOOST_CHECK(!match_begin(s1.begin(), s1.end(), ends.begin(), ends.end()));
    }
  }
  string s2(" s t  r1");
  BOOST_CHECK(equal_strings_as_seq<char>(s1, s2));
  BOOST_CHECK(!equal_strings_as_seq<string>(s1, s2));
  string s3(" s \nt  \tr1 ");
  BOOST_CHECK(equal_strings_as_seq<char>(s2, s3));
  BOOST_CHECK(equal_strings_as_seq<string>(s2, s3));
  string s4("str1a");
  BOOST_CHECK(!equal_strings_as_seq<string>(s1, s4));
  BOOST_CHECK(!equal_strings_as_seq<char>(s1, s4));
  BOOST_CHECK(!equal_strings_as_seq<char>(s4, s1));
  string s5("ab \t\n ");
  string s5t("ab");
  string s6("\t a \r\n");
  string s6c("\t a ");
  string s6t("a");
  string s6rt("\t a");
  string s6lt("a \r\n");
  EXPECT_EQ(s5t, trim(s5));
  EXPECT_EQ(s5t, trim(s5t));
  EXPECT_EQ(s5t, rtrim(s5));
  EXPECT_EQ(s5t, rtrim(s5t));
  EXPECT_EQ(s6t, trim(s6));
  EXPECT_EQ(s6t, trim(s6t));
  EXPECT_EQ(s6rt, rtrim(s6));
  EXPECT_EQ(s6rt, rtrim(s6rt));
  EXPECT_EQ(s6lt, ltrim(s6));
  EXPECT_EQ(s6lt, ltrim(s6lt));
  EXPECT_EQ(s6c, chomp(s6));
  EXPECT_EQ(s6c, chomp(s6c));
  EXPECT_EQ(s6rt, chomp(s6rt));
  EXPECT_EQ("", chomp(string("\n")));
}

#endif

}  // graehl

#endif
