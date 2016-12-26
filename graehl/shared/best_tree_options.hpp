/** \file

 .
*/

#ifndef BEST_TREE_OPTIONS_GRAEHL_2016_10_20_HPP
#define BEST_TREE_OPTIONS_GRAEHL_2016_10_20_HPP
#pragma once

#include <cstddef>
#include <iostream>
#include <string>

namespace graehl {

struct BestTreeStats {
  std::size_t n_blocked_rereach, n_relax, n_update, n_converged_inexact, n_pop, n_unpopped;
  BestTreeStats()
      : n_blocked_rereach(), n_relax(), n_update(), n_converged_inexact(), n_pop(), n_unpopped() {}
  typedef BestTreeStats self_type;
  template <class O>
  void print(O& o) const {
    o << "BestTreeStats:"
      << " blocked " << n_blocked_rereach << " negative-cost improvements"
      << "; evaluated " << n_relax << " edges"
      << " and improved " << n_update << " of those";
    if (n_converged_inexact)
      o << ", skipping re-queueing of " << n_converged_inexact << " by within-epsilon convergence";
    o << "; found best cost of " << n_pop << " vertices";
    if (n_unpopped) o << " and left " << n_unpopped << " reachable vertices unused";
    o << ".";
  }
  friend inline std::ostream& operator<<(std::ostream& out, BestTreeStats const& self) {
    self.print(out);
    return out;
  }
};

struct BestTreeOptions {
  /// allow repeated requeueing of already "best known" popped nodes, this many times
  /// (0 means don't need to track, otherwise we instantiate rereachptr). we count to
  /// avoid infinite loop if neg cost cycle. this count isn't used until we requeue
  /// something we popped; we don't mind if we find new best costs for a tail
  /// repeatedly as long as we didn't pop the head of that edge yet.
  unsigned allow_rereach;

  /// throw exception on excess of allow_rereach if true, otherwise just increment stat.n_blocked_rereach
  bool throw_on_rereach_limit;

  /// if set, stop before allow_rereach limit if
  /// PT::converged(improvement, previous, convergence_epsilon) - that is,
  /// plus(improvement, previous)<convergence_epsilon. string because path_traits
  /// may dictate something other than float
  std::string convergence_epsilon_str;

  BestTreeOptions() { defaults(); }
  void defaults() {
    allow_rereach = 0;
    throw_on_rereach_limit = false;
    convergence_epsilon_str.clear();
  }
  template <class Conf>
  void configure(Conf& c) {
    c("rereach", &allow_rereach)
        .defaulted()(
            "mostly for handling lattices with net-negative-cost edges: in best-first allow the 'best' path "
            "to a node to be reached this many times (may be needed if edge costs are negative more than sum "
            "of best paths to all-but-one tail is positive). if every edge has a net-postivie cost, this can "
            "be 0, which saves memory");
    c("throw-on-max-rereach", &throw_on_rereach_limit)
        .defaulted()(
            "if a node is popped more than rereach+1 times, throw a BestTreeRereachException immediately)");
    c("convergence", &convergence_epsilon_str)
        .defaulted()(
            "if empty or 0, only stop on maximum # of rereaches or 0 change anywhere. else stop if change is "
            "below this amount")
        .is("epsilon - a small nonnegative real");
    c.is("BestTree");
  }
};


}

#endif
