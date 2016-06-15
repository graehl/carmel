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

    make hooking default-constructed compression/decompression filters up to i/o fstream easy
*/

#ifndef FILTER_FILE_STREAM_JG_2015_01_06_HPP
#define FILTER_FILE_STREAM_JG_2015_01_06_HPP
#pragma once

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/traits.hpp>
#include <fstream>
#include <stdexcept>

namespace graehl {

template <class Mode>
struct fstream_for_mode {  // for input
};

template <>  // for input
struct fstream_for_mode<boost::iostreams::input> {  // for input
  typedef std::ifstream stream_type;
  enum { ios_mode_default = std::ios::in };
};


template <>  // for input
struct fstream_for_mode<boost::iostreams::seekable> {
  typedef std::ifstream stream_type;
  enum { ios_mode_default = std::ios::in };
};

template <>  // for output
struct fstream_for_mode<boost::iostreams::output> {
  typedef std::ofstream stream_type;
  enum { ios_mode_default = std::ios::out };
};

template <>  // for output
struct fstream_for_mode<boost::iostreams::output_seekable> {
  typedef std::ofstream stream_type;
  enum { ios_mode_default = std::ios::out };
};

typedef std::runtime_error filter_file_stream_exception;

inline void throw_filter_file_stream_open(char const* name) {
  throw filter_file_stream_exception(std::string("Can't open file: ") + name);
}

/**
   'Filter' is e.g. boost::iostreams::zlib_decompressor or
   boost::iostreams::zlib_compressor from <boost/iostreams/filter/zlib.hpp>

   'Mode': Each mode is represented by a mode tag, defined in the header
   <boost/iostreams/traits.hpp>. There are eight tags for the eight modes:
   input, output, bidirectional, input_seekable, output_seekable, seekable,
   dual_seekable and bidirectional_seekable.[1] As with standard library
   iterator category tags, the tag corresponding to a mode is convertible to
   each of the tags corresponding to modes which the first mode refines.

*/

/**

   filtering_stream derives from std::basic_istream, std::basic_ostream or
   std::basic_iostream, depending on its Mode parameter.

   template< typename Mode,
   typename Ch     = char,
   typename Tr     = std::char_traits<Ch>,
   typename Alloc  = std::allocator<Ch>,
   typename Access = public_ >
   class filtering_stream;
 */
template <class Filter, class Mode = boost::iostreams::input_seekable, class Stream = typename fstream_for_mode<Mode>::stream_type>
struct filter_file_streambuf : boost::iostreams::filtering_streambuf<Mode> {
  typedef boost::iostreams::filtering_streambuf<Mode> Base;
  Stream file_;

  filter_file_streambuf() {}
  filter_file_streambuf(char const* name, std::ios_base::openmode mode = fstream_for_mode<Mode>::ios_mode_default)
      : file_(name, mode | std::ios_base::binary) {
    opened(name);
  }

  ~filter_file_streambuf() {
    Base::reset();  // flush filter buffer before closing output file
  }

  bool is_open() { return file_.is_open(); }

  void open(char const* name, std::ios_base::openmode mode = fstream_for_mode<Mode>::ios_mode_default) {
    file_.open(name, mode | std::ios_base::binary);
    opened(name);
  }
  void open(std::string const& name, std::ios_base::openmode mode = fstream_for_mode<Mode>::ios_mode_default) {
    open(name.c_str(), mode);
  }

  void close() {
    Base::reset();
    file_.close();
  }

  void opened(char const* name) {
    if (!file_) throw_filter_file_stream_open(name);
    Base::push(Filter());
    Base::push(file_);
  }
};

template <class Filter, class Mode = boost::iostreams::input_seekable, class Stream = typename fstream_for_mode<Mode>::stream_type>
struct filter_file_stream : boost::iostreams::filtering_stream<Mode> {
  typedef boost::iostreams::filtering_stream<Mode> Base;
  Stream file_;

  filter_file_stream() {}
  filter_file_stream(char const* name, std::ios_base::openmode mode = fstream_for_mode<Mode>::ios_mode_default)
      : file_(name, mode | std::ios_base::binary) {
    opened(name);
  }

  ~filter_file_stream() {
    Base::reset();  // flush filter buffer before closing output file
  }

  bool is_open() { return file_.is_open(); }

  void open(char const* name, std::ios_base::openmode mode = fstream_for_mode<Mode>::ios_mode_default) {
    file_.open(name, mode | std::ios_base::binary);
    opened(name);
  }
  void open(std::string const& name, std::ios_base::openmode mode = fstream_for_mode<Mode>::ios_mode_default) {
    open(name.c_str(), mode);
  }

  void close() {
    Base::reset();
    file_.close();
  }

  void opened(char const* name) {
    if (!file_) throw_filter_file_stream_open(name);
    Base::push(Filter());
    Base::push(file_);
  }
};


}

#endif
