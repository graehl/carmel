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

    boost large sequential read/write stream performance with a larger than usual buffer.
*/


#ifndef GRAEHL_SHARED__LARGE_STREAMBUF_HPP
#define GRAEHL_SHARED__LARGE_STREAMBUF_HPP
#pragma once

#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <fstream>
#include <new>

namespace graehl {

// must have at least the lifetime of the stream you attach this to (or else call detach(stream) before this
// dies)
template <std::size_t bufsize = 256 * 1024>
struct large_streambuf {
  BOOST_STATIC_CONSTANT(std::size_t, size = bufsize);
  char buf[bufsize];
  large_streambuf() {}
  template <class S>
  large_streambuf(S& s) {
    attach_to_stream(s);
  }
  template <class S>
  void attach_to_stream(S& stream) {
    if (size) stream.rdbuf()->pubsetbuf(buf, size);
  }
  template <class S>
  void detach(S& stream) {
    stream.rdbuf()->pubsetbuf(0, 0);
  }
};


/**
   while this object exists, a basic_ostream or basic_istream will have a large buffer.

   for fstream, you must expand the streambuf *before* opening the file (this
   may be a linux bug), or else the buffer won't be used.

   either this object must last longer than the stream, or call
   resets(Stream *s) so you'll be able to continue using the stream later
*/

struct bigger_streambuf : boost::noncopyable {
  std::size_t size;
  void* buf;
  std::streambuf* resetbuf;

  bigger_streambuf(std::size_t size) : size(size), buf(size ? ::operator new(size) : 0), resetbuf() {}

  template <class S>
  bigger_streambuf(std::size_t size, S& s, bool stream_outlives_buffer = true)
      : size(size), buf(size ? ::operator new(size) : 0) {
    attach_to_stream(s, stream_outlives_buffer);
  }
  template <class S>
  void attach_to_stream(S& s, bool stream_outlives_buffer = true) {
    if (size) {
      resetbuf = stream_outlives_buffer ? s.rdbuf() : 0;
      s.rdbuf()->pubsetbuf((char*)buf, size);
    }
  }
  void reset() {
    if (size) {
      if (resetbuf) resetbuf->pubsetbuf(0, 0);  // this should flush if old buffer wasn't flushed yet
      ::operator delete(buf);
      size = 0;
    }
  }
  ~bigger_streambuf() { reset(); }
};


}

#endif
