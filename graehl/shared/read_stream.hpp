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

    .
*/

#ifndef READ_STREAM_JG_2014_09_05_HPP
#define READ_STREAM_JG_2014_09_05_HPP
#pragma once

#include <cassert>
#include <cstdlib>
#include <sstream>

namespace graehl {

struct SeekException : std::exception {
  char const* what_;
  SeekException(char const* msg = "Seek error") : what_(msg) {}
  char const* what() const throw() { return what_; }
  ~SeekException() throw() {}
};

inline bool seek_ok(std::streambuf::pos_type pos) {
  return pos != (std::streambuf::pos_type)(std::streambuf::off_type)-1;
}

inline void require_seek_ok(std::streambuf::pos_type pos) {
  if (!seek_ok(pos)) throw SeekException();
}

/**
   read whole contents of streambuf 'buf'

   \return size of data malloced and copied into \param[out] buffer unless size
   is 0 (in which case buffer is set to NULL for convenience, so you can blindly free)

   requires put pointer is at end for determining size (e.g. just-written iostream)
*/
inline std::size_t read_streambuf_malloc(std::streambuf& buf, void*& buffer) {
  typedef std::streambuf::pos_type Pos;
  typedef std::streambuf::off_type Off;
  Pos sz = buf.pubseekoff(0, std::ios::end, std::ios_base::out);
  if (sz > 0 && seek_ok(sz) && seek_ok(buf.pubseekpos(0, std::ios_base::in)))
    return buf.sgetn((char*)(buffer = std::malloc(sz)), sz);
  else {
    buffer = 0;
    return 0;
  }
}

/**
   read whole contents of stringstream 'ss', which should be created with
   ios::in and not just ios::out (otherwise we have to call .str() which makes a
   copy)

   \return size of data malloced and copied into \param[out] buffer unless size
   is 0 (in which case buffer is set to NULL for convenience, so you can blindly free)

   works for a just-written stringstream (i.e. requires put pointer is already
   at end)
*/
inline std::size_t read_stringstream_malloc(std::stringstream& ss, void*& buffer) {
  // return read_streambuf_malloc(*ss.rdbuf(), buffer);
  std::size_t const sz = (std::size_t)ss.tellp();
  if (sz) {
    std::istream& s = ss;
    s.read((char*)(buffer = std::malloc(sz)), sz);
    std::size_t const nread = s.gcount();
    if (nread == sz)
      return sz;
    else {
      std::string const& str = ss.str();
      std::size_t const ssz = str.size();
      assert(ssz == sz);
      std::memcpy(buffer, str.data(), sz);
      return sz;
    }
  } else {
    buffer = 0;
    return sz;
  }
}


}

#endif
