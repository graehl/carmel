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
#ifndef GRAEHL__SHARED__THE_NULL_OSTREAM_HPP
#define GRAEHL__SHARED__THE_NULL_OSTREAM_HPP

#include <graehl/shared/null_ostream.hpp>

#ifdef GRAEHL__SINGLE_MAIN
# define GRAEHL__NULL_OSTREAM_MAIN
#endif

#ifdef GRAEHL__NULL_OSTREAM_MAIN
null_ostream the_null_ostream;
#else
/// singleton/constant (only need one)
extern null_ostream the_null_ostream;
#endif

#endif
