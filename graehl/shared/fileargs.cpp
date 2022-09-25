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

    see fileargs.hpp
*/

#include <graehl/shared/fileargs.hpp>

#if GRAEHL_USE_GZSTREAM
#include <graehl/shared/gzstream.hpp>
#endif

namespace graehl {

namespace {
static std::string const fail_in = "Couldn't open compressed input file";
static std::string const fail_out = "Couldn't create compressed output file";
}

template <class Stream>
struct call_set_new_gz {
  template <class Filearg>
  static void gz(Filearg& x, std::string const& s) {
    throw std::runtime_error("can't open .gz as fstream");
  }
#if USE_BOOST_BZ2STREAM
  template <class Filearg>
  static void bz2(Filearg& x, std::string const& s) {
    throw std::runtime_error("can't open .bz2 as fstream");
  }
#endif
};

template <>
struct call_set_new_gz<std::ostream> {
  template <class Filearg>
  static void gz(Filearg& x, std::string const& s) {
    x.template set_new<ogzstream>(s, fail_out);
  }
#if USE_BOOST_BZ2STREAM
  template <class Filearg>
  static void bz2(Filearg& x, std::string const& s) {
    x.template set_new<obz2stream>(s, fail_out);
  }
#endif
};

template <>
struct call_set_new_gz<std::istream> {
  template <class Filearg>
  static void gz(Filearg& x, std::string const& s) {
    x.template set_new<igzstream>(s, fail_in);
  }
#if USE_BOOST_BZ2STREAM
  template <class Filearg>
  static void bz2(Filearg& x, std::string const& s) {
    x.template set_new<ibz2stream>(s, fail_in);
  }
#endif
};


template <class Stream>
void file_arg<Stream>::set_gzfile(std::string const& s, bool /*large_buf*/)
// gzstream has a static 256k buffer already. big enough. //TODO: what about USE_BOOST_GZSTREAM buffer?
{
  try {
    call_set_new_gz<Stream>::gz(*this, s);
  } catch (std::exception& e) {
    throw_fail(s, std::string("-exception: ") + e.what());
  }
}

#if USE_BOOST_BZ2STREAM
template <class Stream>
void file_arg<Stream>::set_gzfile(std::string const& s, bool /*large_buf*/)
// gzstream has a static 256k buffer already. big enough. //TODO: what about USE_BOOST_GZSTREAM buffer?
{
  try {
    call_set_new_gz<Stream>::bz2(*this, s);
  } catch (std::exception& e) {
    throw_fail(s, std::string("-exception: ") + e.what());
  }
}
#endif
#define GRAEHL_INSTANTIATE_SET_GZFILE(Stream) \
  template void file_arg<Stream>::set_gzfile(std::string const&, bool)

GRAEHL_INSTANTIATE_SET_GZFILE(std::istream);
GRAEHL_INSTANTIATE_SET_GZFILE(std::ostream);
GRAEHL_INSTANTIATE_SET_GZFILE(std::ifstream);
GRAEHL_INSTANTIATE_SET_GZFILE(std::ofstream);


}
