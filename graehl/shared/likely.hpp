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

    branch prediction annotations. note: gcc already has some heuristics that guess
    ok. add predictions only if you're sure or you benchmarked.

    usage: if (likely(a>b)) ; // meaning you expect a>b to be true.
*/

#ifndef LIKELY_GRAEHL_2015_10_21_HPP
#define LIKELY_GRAEHL_2015_10_21_HPP
#pragma once

/// standard-ish from linux kernel code but with a safe(ish) longer name:
/// usage: if (likely_true(a>b)) ...
/// meaning you /// expect a>b to be true.
#ifdef _MSC_VER
#define likely_true(x) (x)
#define likely_false(x) (x)
#else
#define likely_true(x) __builtin_expect(!!(x), 1)
#define likely_false(x) __builtin_expect(!!(x), 0)
#endif
#endif
