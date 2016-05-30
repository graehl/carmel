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
#ifndef GRAEHL__SHARED__GZSTREAM_HPP
#define GRAEHL__SHARED__GZSTREAM_HPP
#pragma once

#ifndef USE_BOOST_GZSTREAM
#define USE_BOOST_GZSTREAM 1
#endif

#if USE_BOOST_GZSTREAM || USE_BOOST_BZ2STREAM
#include <graehl/shared/filter_file_stream.hpp>

#if USE_BOOST_GZSTREAM
#include <boost/iostreams/filter/gzip.hpp>
// see also zlib.hpp for (.gz header)-less compression
#endif

#if USE_BOOST_BZ2STREAM
#include <boost/iostreams/filter/bzip2.hpp>
#endif

namespace graehl {

#if USE_BOOST_GZSTREAM
typedef filter_file_stream<boost::iostreams::gzip_decompressor, boost::iostreams::input, std::ifstream> igzstream;
typedef filter_file_stream<boost::iostreams::gzip_compressor, boost::iostreams::output, std::ofstream> ogzstream;
#endif
#if USE_BOOST_BZ2STREAM
typedef filter_file_stream<boost::iostreams::bz2_decompressor, boost::iostreams::input, std::ifstream> ibz2stream;
typedef filter_file_stream<boost::iostreams::bz2_compressor, boost::iostreams::output, std::ofstream> obz2stream;
#endif
}

#else

#include <graehl/shared/gzstream.h>

#if (!defined(GRAEHL__NO_GZSTREAM_MAIN) && defined(GRAEHL__SINGLE_MAIN)) || defined(GRAEHL__GZSTREAM_MAIN)
// FIXME: generate named library/object instead?
#include "gzstream.cpp"
#endif


#endif
#endif
