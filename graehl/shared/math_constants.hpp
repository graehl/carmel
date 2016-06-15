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

    (double precision) floating point constants. like you'd get from gcc math.h
    in C but not consistently available w/ MSVC10
*/
#pragma once

#include <cmath>

#ifndef _MATH_DEFINES_DEFINED
#define _MATH_DEFINES_DEFINED

#define STATIC_CONST_GLOBAL(x) __declspec(selectany) extern const x

/**
        M_E        e
        M_LOG2E    log2(e)
        M_LOG10E   log10(e)
        M_LN2      ln(2)
        M_LN10     ln(10)
        M_PI       pi
        M_PI_2     pi/2
        M_PI_4     pi/4
        M_1_PI     1/pi
        M_2_PI     2/pi
        M_2_SQRTPI 2/sqrt(pi)
        M_SQRT2    sqrt(2)
        M_SQRT1_2  1/sqrt(2)
 */
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340736
#endif
#ifndef M_LOG10E
#define M_LOG10E 0.434294481903251827651
#endif
#ifndef M_LN2
#define M_LN2 0.693147180559945309417
#endif
#ifndef M_LN10
#define M_LN10 2.30258509299404568402
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4 0.785398163397448309616
#endif
#ifndef M_1_PI
#define M_1_PI 0.318309886183790671538
#endif
#ifndef M_2_PI
#define M_2_PI 0.636619772367581343076
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524401
#endif

#endif
