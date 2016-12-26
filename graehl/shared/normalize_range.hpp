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

    (with options for add-k smoothing) normalize counts into probabilities
*/

#ifndef GRAEHL_SHARED__NORMALIZE_RANGE_HPP
#define GRAEHL_SHARED__NORMALIZE_RANGE_HPP
#pragma once

#include <boost/range.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/value_type.hpp>

namespace graehl {

template <class R>
typename boost::range_value<R>::type sum(R const& r, typename boost::range_value<R>::type z
                                                     = typename boost::range_value<R>::type()) {
  return boost::accumulate(r, z);
}

template <class W>
struct normalize_options {
  W addk_num;
  W addk_denom;
  explicit normalize_options(W addk_num = W(), W addk_denom = W())
      : addk_num(addk_num), addk_denom(addk_denom) {}
  void addUnseen(W n) {
    addk_denom += n * addk_num;  // there are n tokens with 0 count; intend addk_num to apply to them also
  }
  static char const* caption() { return "Probability Normalization"; }
  template <class Config>
  void configure(Config& c) {
    c('k')("addk,k", &addk_num)("add counts of [addk] to every observed event before normalizing");
    c('K')("denom-addk,K", &addk_denom)("add [denom-addk] to the denominator only when normalizing");
    c.is("add-k normalization");
  }
};

template <class W>
struct normalize_addk {
  W knum;
  W denom;
  normalize_addk(std::size_t N, W sum_unsmoothed, W addk_num = 0, W addk_denom = 0) : knum(addk_num) {
    denom = sum_unsmoothed + knum * N + addk_denom;
  }
  normalize_addk(std::size_t N, W sum_unsmoothed, normalize_options<W> const& n) : knum(n.addk_num) {
    denom = sum_unsmoothed + knum * N + n.addk_denom;
  }
  W operator()(W c) const {
    return (c + knum) / denom;
  }  // maybe some semirings don't have * (1/denom) but have / denom ?
};

template <class R, class O>
O normalize_copy(typename boost::range_value<R>::type sum, R const& r, O o,
                 normalize_options<typename boost::range_value<R>::type> n
                 = normalize_options<typename boost::range_value<R>::type>()) {
  typedef typename boost::range_value<R>::type V;
  return boost::transform(r, o, normalize_addk<V>(boost::size(r), sum, n));
}

template <class R, class O>
O normalize_copy(R const& r, O o, normalize_options<typename boost::range_value<R>::type> n
                                  = normalize_options<typename boost::range_value<R>::type>()) {
  return normalize_copy(sum(r), r, boost::begin(r), n);
}

template <class R>
void normalize_sum(typename boost::range_value<R>::type sum, R& r,
                   normalize_options<typename boost::range_value<R>::type> n
                   = normalize_options<typename boost::range_value<R>::type>()) {
  normalize_copy(sum, r, boost::begin(r), n);
}

template <class R>
void normalize(R& r, normalize_options<typename boost::range_value<R>::type> n
                     = normalize_options<typename boost::range_value<R>::type>()) {
  normalize_sum(sum(r), r, n);
}


}

#endif
