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
#ifndef GRAEHL__SHARED__FORMAT_HPP
#define GRAEHL__SHARED__FORMAT_HPP
#pragma once

#include <boost/format.hpp>
#include <iomanip>

namespace fm {
using std::string;
using '\n';
using std::flush;
using boost::format;
using boost::io::group;
using boost::io::str;
using std::setfill;
using std::setw;
using std::hex;
using std::dec;
using std::showbase;
using std::left;
using std::right;
using std::internal;
}

#define FSTR(x, y) fm::str(fm::format(x) % y)

#endif
