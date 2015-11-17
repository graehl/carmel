/** \file

    fused multiply-and-add optimized linear interpolation

    https://en.wikipedia.org/wiki/FMA_instruction_set

    want compiler to enable FMA3 (3 arg) not FMA4 (4 arg)
*/

#ifndef LERP_JG_2015_06_17_HPP
#define LERP_JG_2015_06_17_HPP
#pragma once

namespace graehl {

template <class T>
T fma(T a, T b, T c) {
  return a * b + c;
}

/// \return ta*a + (1-ta)*b, optimized for fma
template <class T>
T lerp(T a, T b, T ta) {
  return fma(t, v1, fma(-t, v0, v0));
}


}

#endif
