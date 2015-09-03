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
/**
   \file

   Traits for (n-)best paths of graphs/hypergraphs (useful for lazy_forest_kbest.hpp). Note: Your cost_type
   may be a<b means a better than b (mostly to simplify) or you can override better_cost(). better is used to
   order the heap and test convergence. updates is the same for viterbi but may always return true if you want
   to sum all paths? unsure.
 */

#ifndef GRAEHL_SHARED__PATH_TRAITS_HPP
#define GRAEHL_SHARED__PATH_TRAITS_HPP
#pragma once

#include <boost/graph/graph_traits.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <graehl/shared/epsilon.hpp>
#include <graehl/shared/nan.hpp>

namespace graehl {

// for additive costs (lower = better)
template <class Float = float>
struct cost_path_traits {
  typedef Float cost_type;
  static const bool viterbi
      = true;  // means updates() sometimes returns false. a<b with combine(a, a)=a would suffice
  static inline cost_type unreachable() { return std::numeric_limits<cost_type>::infinity(); }
  static inline cost_type start() { return 0; }
  static inline cost_type extend(cost_type a, cost_type b) { return a + b; }
  static inline cost_type extendBy(cost_type delta, cost_type& b) { return b += delta; }
  static inline cost_type retract(cost_type a, cost_type b) { return a - b; }
  static inline cost_type combine(cost_type a, cost_type b) { return std::min(a, b); }
  static inline bool better(cost_type a, cost_type const& b) { return a < b; }
  static inline bool update(cost_type candidate, cost_type& best) {
    if (candidate < best) {
      best = candidate;
      return true;
    }
    return false;
  }
  static inline bool updates(cost_type candidate, cost_type best) { return candidate < best; }
  static inline cost_type repeat(cost_type a, float n) { return a * n; }
  static inline bool includes(cost_type candidate,
                              cost_type best) {  // update(you can assert this after update)
    return !(candidate < best);
  }
  /**
     \return candidate isn't (much) better (lower) than best
  */
  static inline bool includes(cost_type candidate, cost_type best, float delta_relative) {
    assert(delta_relative >= 0);
    float delta = delta_relative;
    if (candidate < 0)  // relative delta; unweighted (relative to 1) if 0 candidate
      delta *= -candidate;
    if (candidate > 0) delta *= candidate;
    return !(candidate + delta < best);
  }
  /**
     \return a and b
  */
  static inline bool close_enough(cost_type a, cost_type b) {
    return few_ieee_apart_equate_inf_nan(a, b, 200);
    // 200 floats distance ~ 1 part in 50,000
  }
  // may be different from includes in the same way that better is different from update:
  static inline bool converged(cost_type improver, cost_type incumbent, cost_type epsilon) {
    return includes(improver, incumbent,
                    epsilon);  // may be different for other cost types (because float may not = cost_Type)
  }
};

template <class G>
struct path_traits : cost_path_traits<float> {};

/*
  template <class G>
  static inline bool converged(typename path_traits<G>::cost_type const& improver, typename
  path_traits<G>::cost_type const& incumbent
  ,typename path_traits<G>::cost_type const& epsilon)
  {
  typedef path_traits<G> PT;
  return PT::better(incumbent, PT::combine(improver, epsilon));
  }
*/

// for graphs which have edges with sources (plural) and not source - ordered multihypergraphs
template <class G>
struct edge_traits {
  typedef typename path_traits<G>::cost_type cost_type;
  typedef boost::graph_traits<G> GT;
  typedef unsigned tail_descriptor;
  typedef boost::counting_iterator<tail_descriptor> tail_iterator;
  typedef unsigned tails_size_type;  // must always be unsigned. for now
};

/*
  free fns (ADL):

  none. statics in path_traits, instead
*/


template <class G>
struct updates_cost {
  typedef path_traits<G> PT;
  typedef typename PT::cost_type cost_type;
  typedef bool result_type;
  bool operator()(cost_type const& a, cost_type const& b) const { return PT::updates(a, b); }
};

template <class G>
struct better_cost {
  typedef path_traits<G> PT;
  typedef typename PT::cost_type cost_type;
  typedef bool result_type;
  bool operator()(cost_type const& a, cost_type const& b) const { return PT::better(a, b); }
};


}

#endif
