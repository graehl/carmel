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

    an ostreambuf that writes to two ostreambufs

    like unix "tee" for ostreams - see test main() program below
*/

#ifndef GRAEHL__SHARED__TEESTREAM_HPP
#define GRAEHL__SHARED__TEESTREAM_HPP
#include <iostream>
#pragma once
namespace graehl {
class teebuf : public std::streambuf {
 public:
  typedef std::char_traits<char> traits_type;
  typedef traits_type::int_type int_type;

  teebuf(std::streambuf* sb1, std::streambuf* sb2) : sb1_(sb1), sb2_(sb2) {}
  int_type overflow(int_type c) override {
    if ((sb1_ && sb1_->sputc(c) == traits_type::eof()) || (sb2_ && sb2_->sputc(c) == traits_type::eof()))
      return traits_type::eof();
    return c;
  }

 private:
  std::streambuf* sb1_;
  std::streambuf* sb2_;
};
}

#ifdef GRAEHL_TEST_MANUAL
#include <fstream>
int main() {
  std::ofstream logfile("/tmp/logfile.txt");
  graehl::teebuf teebuf(logfile.rdbuf(), std::cerr.rdbuf());
  std::ostream log(&teebuf);
  // write log messages to 'log'
  log << "Hello, dude.  check /tmp/logfile.txt\n";
}
#endif


#endif
