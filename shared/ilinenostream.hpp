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
// input stream that tracks the current line number
#ifndef LINENOSTREAM_HPP
#define LINENOSTREAM_HPP
#include <steambuf>
#include <istream>

namespace graehl {

struct linenobuf:
      public std::streambuf
{
  linenobuf(std::streambuf* sbuf): sbuf_(sbuf), lineno_(1) {}
  int lineno() const { return lineno_; }

 private:
  int_type underflow() { return sbuf_->sgetc(); }
  int_type uflow()
  {
    int_type rc = sbuf_->sbumpc();
    if (traits_type::eq_int_type(rc, traits_type::to_int_type('\n')))
      ++lineno_;
    return rc;
  }

  std::streambuf* sbuf_;
  int lineno_;
};

/* Thanks to Dietmar Kuehl:

   Instead of dealing with each
   character individually, you can setup a buffer in the 'underflow()'
   method (see 'std::streambuf::setg()') which is filled using 'sgetn()'
   on the underlying stream buffer. In this case you would not use 'uflow()'
   at all (this method is only used for unbuffered input stream buffers).
   A huge buffer is read into the internal buffer which is chopped up at
   line breaks such that 'underflow()' is called if a line break is hit.
*/

struct ilinenostream:
      public std::istream
{
  ilinenostream(std::istream& stream):
      std::ios(&sbuf_),
      std::istream(&sbuf_)
      sbuf_(stream.rdbuf())
  {
    init(&sbuf_);
  }

  int lineno() { return sbuf_.lineno(); }

 private:
  linenobuf sbuf_;
};

#ifdef GRAEHL_TEST
#include <fstream>
int main() {
  return 0;
}
#endif
}

#endif
