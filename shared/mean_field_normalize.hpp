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
#ifndef GRAEHL__SHARED__MEAN_FIELD_NORMALIZE_HPP
#define GRAEHL__SHARED__MEAN_FIELD_NORMALIZE_HPP

#include <digamma.hpp>
#include <weight.h>

namespace graehl {

struct mean_field_scale
{
  bool linear; // if linear, then don't use alpha.  otherwise convert to exp(digamma(alpha+x))
  double alpha;

  // returns exp(digamma(x))
  template <class Real>
  logweight<Real> operator()(logweight<Real> const& x) const
  {
    if (linear)
      return x;
    double r = x.getReal();
    if (x < .0001) // until we can compute digamma in logspace, this will be the answer.  and, can't ask digamma(0), because it's negative inf.  but exp(-inf)=0
      return 0;
    logweight<Real> ret;
    ret.setLn(digamma(alpha+r));
  }
};

}


#endif
