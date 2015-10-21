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
