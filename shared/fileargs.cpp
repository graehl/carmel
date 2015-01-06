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

    see fileargs.hpp
*/

#include <graehl/shared/fileargs.hpp>

#if GRAEHL_USE_GZSTREAM
#include <graehl/shared/gzstream.hpp>
#endif

namespace graehl {

namespace {
const std::string fail_in="Couldn't open compressed input file";
const std::string fail_out="Couldn't create compressed output file";
}

template <class Stream>
void file_arg<Stream>::set_gzfile(std::string const& s, bool /*large_buf*/)
// gzstream has a static 256k buffer already. big enough.
{
  const bool read=stream_traits<Stream>::read;
  std::string fail_msg;
  try {
    if (read) {
      fail_msg=fail_in;
      set_new<igzstream>(s, fail_msg);
    } else {
      fail_msg=fail_out;
      set_new<ogzstream>(s, fail_msg);
    }
  } catch (std::exception &e) {
    fail_msg.append(" - exception: ").append(e.what());
    throw_fail(s, fail_msg);
  }
}

#define GRAEHL_INSTANTIATE_SET_GZFILE(Stream) template void file_arg<Stream>::set_gzfile(std::string const&,bool)

GRAEHL_INSTANTIATE_SET_GZFILE(std::istream);
GRAEHL_INSTANTIATE_SET_GZFILE(std::ostream);
GRAEHL_INSTANTIATE_SET_GZFILE(std::ifstream);
GRAEHL_INSTANTIATE_SET_GZFILE(std::ofstream);


}
