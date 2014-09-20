#include "weight.h"

namespace graehl {

template <class Real>
const Real logweight<Real>::ln10 = std::log(10.f);
template <class Real>
const Real logweight<Real>::neg_ln10 = -std::log(10.f);
template <class Real>
const Real logweight<Real>::oo_ln10 = 1./std::log(10.f);
template <class Real>
const Real logweight<Real>::neg_oo_ln10 = -1./std::log(10.f);
template <class Real>
const int logweight<Real>::base_index = std::ios_base::xalloc();
// xalloc gives a unique global handle with per-ios space handled by the ios
template <class Real>
const int logweight<Real>::thresh_index = std::ios_base::xalloc();
template <class Real>
THREADLOCAL int logweight<Real>::default_base = logweight<Real>::EXP;
template <class Real>
THREADLOCAL int logweight<Real>::default_thresh = logweight<Real>::ALWAYS_LOG;
}
