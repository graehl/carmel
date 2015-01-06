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
#ifndef GRAEHL__SHARED__MEAN_FIELD_SCALE_HPP
#define GRAEHL__SHARED__MEAN_FIELD_SCALE_HPP

#include <graehl/shared/digamma.hpp>
#include <graehl/shared/weight.h>

namespace graehl {

struct mean_field_scale
{
  bool linear; // if linear, then don't use alpha.  otherwise convert to exp(digamma(alpha+x))
  double alpha;
  mean_field_scale() { set_default(); }
  void set_default()
  {
    linear = true;
    alpha = 0;
  }
  void set_alpha(double a)
  {
    linear = false;
    alpha = a;
  }

  // returns exp(digamma(x))
  template <class Real>
  logweight<Real> operator()(logweight<Real> const& x) const
  {
    if (linear)
      return x;
    typedef logweight<Real> W;
    double xa = x.getReal()+alpha;
    const double floor = .0002;
    static const W dig_floor(digamma(floor), false);
    if (xa < floor) // until we can compute digamma in logspace, this will be the answer.  and, can't ask digamma(0), because it's negative inf.  but exp(-inf)=0
      return dig_floor*(xa/floor);
    // this is a mistake: denominator of sum of n things is supposed to get (alpha*n + sum), not (alpha+sum).  but it seems to work better (sometimes)
    return W(digamma(xa), false);
  }
};

}


#endif
