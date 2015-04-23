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
#ifndef GRAEHL__SHARED__LAZY_FOREST_KBEST_HPP
#define GRAEHL__SHARED__LAZY_FOREST_KBEST_HPP
#pragma once

// you may override this with a fully namespace qualified type - but be careful to do so consistently before
// every inclusion!
// in practice unsigned would be great. who visits > 4billion best?
#ifndef LAZY_FOREST_KBEST_SIZE
#define LAZY_FOREST_KBEST_SIZE unsigned
#endif
// TODO: cycles - if you found a pending, then try not queueing successors until you're added to memo table.
// but MAYBE our successors first approach makes us handle negative cost improvements more nicely? not sure.
// if not, then always queue successors afterwards.

// FIXME: see postpone_selfloop for weakness in current cycle handling - apparently want a non-cycle 2nd best,
// then it's sort of ok?

/**

   Uses a top-down, lazy, best-first kbest-derivation from a forest algorithm
   described at http://www.cis.upenn.edu/~lhuang3/huang-iwpt-correct.pdf.
   Generating, say, the 1-million best should be no problem.

   Every non-optimal kth-best derivation uses either a non-best edge, or a best
   edge but using nonoptimal subderivations. Typically, the kbest list will
   contain only a small number of unique "sidetracks" (non-optimal edges), but
   may choose different subsets of them (should they occur in different
   subtrees, their use is nonconflicting). So, it is typically best to integrate
   models into search, or at least provide some forest expansion rescoring, as
   the size of the kbest list needed to get the desired variation may be huge.

   a lazy_forest (graehl/shared/lazy_forest_kbest.hpp) that is essentially a
   (non-lazy) copy of the parse forest, with per-or-node priority queues and
   memo facilitating the lazy kbest generation.

   You can generate kbest items incrementally, on demand.

   On consistency: you must populate the priority queues of the lazy_forest such
   that the first best (memo[0] and pq[0]) is the same as is in your "best
   derivation" object. If you use add_sorted, first adding the best derivation
   for each node, then that suffices. If you use sort(), then you probably need
   to adjust all the derivations with fix_edge to be consistent with the result.

   Subtle point: you may end up violating the heap property depending on how
   fix_edge works. However, I'm certain that d_ary_heap.hpp doesn't frivolously
   check for heap property violations.

   TODO: member pointer or recursive function argument pointer to context: deriv
   factory, filter, stats. currently is thread-unsafe due to static globals.

   TODO: resolve pq vs 1best indeterminacy by forcing user to specify which is
   best (default = min cost in forest node), then store "to be expanded"
   hyperedge outside of queue? the fix-best approach may potentially improve the
   1best slightly.
**/

/*
  struct Factory
  {
  typedef Result *derivation_type;
  static derivation_type NONE() { return (derivation_type)0;}
  static derivation_type PENDING() { return (derivation_type)1;}
  derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child,
  lazy_kbest_index_type changed_child_index)
  {
  return new Result(prototype, old_child, new_child, changed_child_index);
  }
  void fix_edge(derivation_type &wrong_1best, derivation_type correct_unary_best); // repair errors in
  non-sorted 1-best derivation
  void fix_edge(derivation_type &wrong_1best, derivation_type correct_left_best, derivation_type
  correct_right_best); // repair errors in non-sorted 1-best derivation
  };

  // you also need operator == (used only for assertions)

  bool derivation_better_than(derivation_type a, derivation_type b);

  or

  // Result(a) < Result(b) iff a is better than b.

  or specialize in ns graehl:

  namespace graehl {
  template<>
  struct lazy_kbest_derivation_traits<Deriv> {
  static inline bool better_than(const Deriv &candidate, const Deriv &than)
  {
  return candidate<than;
  }
  {

  then build a lazy_forest<Factory> binary hypergraph
*/

// TODO: use d_ary_heap.hpp (faster than binary)

#ifndef GRAEHL_DEBUG_LAZY_FOREST_KBEST
#define GRAEHL_DEBUG_LAZY_FOREST_KBEST 0
#endif
#include <graehl/shared/ifdbg.hpp>

#ifndef ERRORQ
#include <iostream>
#define KBESTERRORQ(x, y)                        \
  do {                                           \
    std::cerr << "\n" << x << ": " << y << '\n'; \
  } while (0)
#else
#define KBESTERRORQ(x, y) ERRORQ(x, y)
#endif


#include <graehl/shared/os.hpp>
#include <graehl/shared/containers.hpp>
#include <memory>

#include <boost/noncopyable.hpp>
#include <graehl/shared/percent.hpp>
#include <graehl/shared/assertlvl.hpp>

#include <cstddef>
#include <vector>
#include <stdexcept>
#include <algorithm>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
# define LAZY_FOREST_EXAMPLES
#endif

#if !GRAEHL_DEBUG_LAZY_FOREST_KBEST
# define LAZYF(x)
#else
# include <graehl/shared/show.hpp>
# define LAZYF(x) x
DECLARE_DBG_LEVEL(LAZYF)
# ifdef LAZY_FOREST_KBEST_MSG
#  define KBESTINFOT(x) LAZY_FOREST_KBEST_MSG(x << '\n')
# if defined(NESTT)
#  define KBESTNESTT NESTT
# else
#  ifdef _MSC_VER
#   define KBESTNESTT
#  else
#   define KBESTNESTT SCOPED_INDENT_NEST(lazy_forest_indent_tag)
#  endif
# endif
# else
#  define KBESTINFOT(x)
#  define KBESTNESTT
# endif


# include <graehl/shared/indent_level.hpp>

namespace graehl {
struct lazy_forest_indent_tag {};
# define LAZY_FOREST_KBEST_INDENT SCOPED_INDENT(lazy_forest_indent_tag)
}

# ifndef LAZY_FOREST_KBEST_MSG
#  define LAZY_FOREST_KBEST_MSG(msg) SHOWP(LAZYF, LAZY_FOREST_KBEST_INDENT << msg)
# endif
#endif


#include <graehl/shared/warning_compiler.h>
CLANG_DIAG_OFF(unused-variable)
namespace graehl {

typedef LAZY_FOREST_KBEST_SIZE lazy_kbest_index_type;

inline std::ostream& operator<<(std::ostream& o, lazy_kbest_index_type bp[2]) {
  return o << '[' << bp[0] << ',' << bp[1] << ']';
}

/*
  template <class Deriv> inline
  bool derivation_better_than(const Deriv &me, const Deriv &than)
  {
  return me > than;
  }
*/

// you should probably only override (i.e. specialize) this... not sure how ADL will work. default is smaller
// (e.g. lower cost) is better.
template <class Deriv>
struct lazy_kbest_derivation_traits {
  // Deriv(a) < Deriv(b) iff b is better than a.
  static inline bool better_than(const Deriv& candidate, const Deriv& than) { return candidate < than; }
};

/// but if you overload this, then you don't need to specialize above. this is what we actually call
template <class Deriv>
inline bool derivation_better_than(const Deriv& candidate, const Deriv& than) {
  return lazy_kbest_derivation_traits<Deriv>::better_than(candidate, than);
}

/// but if you overload this, then you don't need to specialize above. this is what we actually call
template <class Deriv>
inline bool call_derivation_better_than(const Deriv& candidate, const Deriv& than) {
  return derivation_better_than(candidate, than);  // ADL here
}

struct lazy_derivation_cycle : public std::runtime_error {
  lazy_derivation_cycle()
      : std::runtime_error(
            "lazy_forest::get_best tried to compute a derivation that eventually depended on itself - "
            "probable cause: negative cost cycle") {}
};

/**
   Note: unfortunately, this entire code does not allow for multiple concurrent jobs (per type of factory),
since there is a static set_derivation_factory. Change would be simple but consume a little extra memory (at
each kbest node?)

   build a copy of your (at most binary) derivation forest, then query its root for the 1st, 2nd, ... best
   <code>
   struct DerivationFactory
   {


   /// can override derivation_better_than(derivation_type, derivation_type).
   /// derivation_type should be a lightweight (value) object
   /// if LAZY_FOREST_KBEST_MSG debug prints are enabled, must also have o << deriv
   typedef Result *derivation_type;

   friend bool derivation_better_than(derivation_type a, derivation_type b); // override this or
lazy_kbest_derivation_traits::better_than. default is a>b

   /// special derivation values (not used for normal derivations) (may be of different type but convertible
to derivation_type)
   static derivation_type PENDING();
   static derivation_type NONE();
   /// derivation_type must support: initialization by (copy), assignment, ==, !=

   /// take an originally better (or best) derivation, substituting for the changed_child_index-th child
new_child for old_child (cost difference should usually be computed as (cost(new_child) - cost (old_child)))
   derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child,
lazy_kbest_index_type changed_child_index)
   {
   return new Result(prototype, old_child, new_child, changed_child_index);
   }
   };
   </code>
**/
// TODO: implement unique visitor of all the lazykbest subresults (hash by pointer to derivation?)

struct none_t {};
struct pending_t {};

struct dummy_init_type {
  template <class Init>
  dummy_init_type(Init const& g) {}
};

// your factory may inherit form this - the do-nothing fix-best should suffice if you're using add_sorted
// only.
struct lazy_kbest_derivation_factory_base {
  template <class A>
  void fix_edge(A& a, A const& unary) {}
  template <class A>
  void fix_edge(A& a, A const& left, A const& right) {}
  template <class A>
  bool needs_fixing(A& a) {
    return false;
  }
};

struct permissive_kbest_filter {
  enum { trivial = 1 };  // avoids printing stats on % filtered
  /*
    typedef some_stateful_derivation_predicate filter_type;
    some_type filter_init(); /// used like: typename factory_type::filter_type
    per_node_filter(factory.filter_init());
  */
  struct dummy_init_type {
    template <class Init>
    dummy_init_type(Init const& g) {}
  };

  typedef dummy_init_type init_type;
  permissive_kbest_filter() {}
  permissive_kbest_filter(init_type const& g) {}

  template <class E>
  bool permit(E const& e) const {
    return true;
  }

  template <class O, class E>
  friend inline void print(O& o, E const& e, permissive_kbest_filter const&) {
    o << e;
  }
};

struct permissive_kbest_filter_factory {
  typedef permissive_kbest_filter filter_type;
  bool filter_init() const  // gets passed to constructor for filter_type
  {
    return true;
  }
};

template <class filter>
struct default_filter_factory {
  typedef filter filter_type;
  filter filter_init() const { return filter(); }
};

// if you defined a filter which is copyable and supply an empty-state prototype to the factory:
template <class filter>
struct copy_filter_factory {
  typedef filter filter_type;
  filter f;
  copy_filter_factory() {}
  copy_filter_factory(filter const& f) : f(f) {}
  copy_filter_factory(copy_filter_factory const& o) : f(o.f) {}
  filter const& filter_init() const { return f; }
};

// a system that passes in projections of children for optimization (maybe avoiding quadratic in sz(deriv)
// work). but hash key len will grow anyway.
template <class Deriv, class Projection, class MapSelect = UmapS>
struct projected_duplicate_filter : Projection {
  typedef projected_duplicate_filter<Deriv, Projection, MapSelect> self_type;
  enum { trivial = 0 };
  projected_duplicate_filter(projected_duplicate_filter const& o) : Projection(o), maxreps(o.maxreps) {}
  projected_duplicate_filter(unsigned maxreps = 0, Projection const& projection = Projection())
      : Projection(projection), maxreps(maxreps) {}
  unsigned maxreps;
  typedef Deriv derivation_type;
  typedef typename Projection::result_type projected_type;
  typedef typename MapSelect::template map<projected_type, unsigned>::type projecteds_type;
  projecteds_type projecteds;
  bool permit(derivation_type const& d) {
    if (!maxreps) return true;
    projected_type const& projected = Projection::operator()(d);
    unsigned n = ++projecteds[projected];
    EIFDBG(LAZYF, 3, std::ostringstream projstr; print_projection(projstr, d);
           SHOWM3(LAZYF, "dup-filter permit?", projstr.str(), n, maxreps));
    return n <= maxreps;
  }
  template <class O>
  void print_projection(O& o, derivation_type const& d) const {
    Projection::print_projection(o, d);
  }
  /*  template <class O>
      void print(O &o, derivation_type const& d, projected_type const& proj)
      {
      o << "projection for "<<&d << ": "<<&proj;
      }*/

  template <class O>
  friend inline void print(O& o, derivation_type const& d, self_type const& s) {
    s.print_projection(o, d);
  }
};

struct lazy_kbest_stats {
  typedef std::size_t count_t;
  /// number generated passed and visited at the root
  count_t n_visited;

  /// passed vs. filtered = total (generated at all levels of forest, not just root)
  count_t n_passed;
  count_t n_filtered;
  bool trivial_filter;
  void clear(bool trivial_filt = false) {
    trivial_filter = trivial_filt;
    n_passed = n_filtered = n_visited = 0;
  }
  count_t n_total() const { return n_passed + n_filtered; }
  lazy_kbest_stats() { clear(); }

  template <class C, class T>
  void print(std::basic_ostream<C, T>& o) const {
    o << "[Lazy kbest visited " << n_visited << " derivations; ";
    if (trivial_filter) {
      assert(!n_filtered);
      o << n_passed << " derivations found]";
      return;
    }
    o << "uniqueness-filtered " << n_filtered << " of " << n_total() << " derivations, leaving " << n_passed
      << ", or " << graehl::percent<5>((double)n_passed, (double)n_total()) << "]";
  }
  typedef lazy_kbest_stats self_type;
};

template <class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& os, lazy_kbest_stats const& kb) {
  kb.print(os);
  return os;
}

namespace {
template <class A, class B, class C>
void adl_print(A& a, B const& b, C const& c) {
  print(a, b, c);
}
template <class A, class B, class C, class D>
void adl_print(A& a, B const& b, C const& c, D const& d) {
  print(a, b, c, d);
}
}

// i've left this copyable even though you should for efficiency's sake only copy empty things, e.g. if you
// want to compile a vector of them that's fine
template <class DerivationFactory, class FilterFactory = permissive_kbest_filter_factory>
class lazy_forest : public FilterFactory::filter_type  // empty base class opt. - may have state e.g. hash of
                    // seen strings or trees
                    {

 public:

#if __cplusplus >= 201103L 
  /// move
  lazy_forest(lazy_forest&& o) noexcept : pq(std::move(o.pq)), memo(std::move(o.memo)) {
  }

  /// move
  lazy_forest& operator=(lazy_forest&& o) noexcept {
    assert(&o != this);
    pq = std::move(o.pq);
    memo = std::move(o.memo);
    return *this;
  }
#endif

  typedef DerivationFactory derivation_factory_type;
  typedef FilterFactory filter_factory_type;
  /**
     formerly global; now thread safe by passing Environment & around. this is
     the minimal refactor that achieves the goal; an alternative would be to put
     more of the code inside this struct (which manages individual lazy_forest
     objects) and make it the primary interface
  */
  struct Environment {
    derivation_factory_type derivation_factory;
    filter_factory_type filter_factory;
    lazy_kbest_stats stats;
    void set_derivation_factory(derivation_factory_type const& df) { derivation_factory = df; }
    void set_filter_factory(filter_factory_type const& f) { filter_factory = f; }
    bool throw_on_cycle;
    Environment() : throw_on_cycle(true) {}
  };

  typedef typename filter_factory_type::filter_type filter_type;
  typedef lazy_forest<derivation_factory_type, filter_factory_type> forest;
  typedef forest self_type;

  explicit lazy_forest(Environment const& env) : filter_type(env.filter_factory.filter_init()) {}

  typedef typename derivation_factory_type::derivation_type derivation_type;

  static inline derivation_type NONE() { return (derivation_type)DerivationFactory::NONE(); }
  static inline derivation_type PENDING() { return (derivation_type)DerivationFactory::PENDING(); }

  void clear_stats(Environment& env) { env.stats.clear(filter_type::trivial); }

  /// bool Visitor(derivation, ith) - if returns false, then stop early.
  /// otherwise stop after generating up to k (up to as many as exist in
  /// forest)
  template <class Visitor>
  lazy_kbest_stats enumerate_kbest(Environment& env, lazy_kbest_index_type k, Visitor visit = Visitor()) {
    EIFDBG(LAZYF, 1, KBESTINFOT("COMPUTING BEST " << k << " for node " << *this); KBESTNESTT;);
    clear_stats(env);
    lazy_kbest_index_type i = 0;
    for (; i < k; ++i) {
      EIFDBG(LAZYF, 2, SHOWM2(LAZYF, "enumerate_kbest-pre", i, *this));
      derivation_type ith_best = get_best(env, i);
      if (ith_best == NONE()) break;
      if (!visit(ith_best, i)) break;
    }
    env.stats.n_visited = i;
    return env.stats;
  }

  void swap(self_type& o) {
    pq.swap(o.pq);
    memo.swap(o.memo);
  }

  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, lazy_forest const& self) {
    self.print(o);
    return o;
  }

  // FIXME: faster heap operations if we put handles to hyperedges on heap instead of copying? boost object
  // pool?
  struct hyperedge {
    typedef hyperedge self_type;
    /// antecedent subderivation forest (OR-nodes). if unary,
    /// child[1]==NULL. leaves have child[0]==NULL also.
    forest* child[2];
    /// index into kth best for that child's OR-node: which of the possible
    /// children we chose (0=best-cost, 1 next-best ...)
    lazy_kbest_index_type childbp[2];
    /// the derivation that resulted from choosing the childbp[0]th out of
    /// child[0] and childbp[1]th out of child[1]
    derivation_type derivation;

    bool self_loop(forest const* self) const {
      if (!child[1]) return false;
      return child[0] == self;
    }

    hyperedge(derivation_type _derivation, forest* c0, forest* c1) { set(_derivation, c0, c1); }
    void set(derivation_type _derivation, forest* c0, forest* c1) {
      childbp[0] = childbp[1] = 0;
      child[0] = c0;
      child[1] = c1;
      derivation = _derivation;
    }
    // NB: std::pop_heap puts largest element at top (max-heap)
    inline bool
    operator<(const hyperedge& o) const {  // true if o should dominate max-heap, i.e. o is better than us
      return call_derivation_better_than(o.derivation, derivation);
    }
    // means: this<o iff o better than this. good.

    template <class O>
    void print(O& o) const {
      o << "{hyperedge(";
      if (child[0]) {
        o << child[0] << '[' << childbp[0] << ']';
        if (child[1]) o << "," << child[1] << '[' << childbp[1] << ']';
      }
      o << ")=" << derivation;
      o << '}';
    }
    template <class Ch, class Tr>
    friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, hyperedge const& self) {
      self.print(o);
      return o;
    }

    bool operator==(hyperedge const& o) const {
      if (o.child[0] != child[0]) return false;
      if (o.child[1] != child[1]) return false;
      if (o.childbp[0] != childbp[0]) return false;
      if (o.childbp[1] != childbp[1]) return false;
      if (o.derivation != derivation) return false;
      return true;
    }
  };

  static inline derivation_type get_child_best(forest const& f) {
    return f.pq.empty() ? NONE() : f.pq[0].derivation;
  }

  void fix_hyperedge(Environment& env, hyperedge& edge, bool dfs = true) {
    forest* c0 = edge.child[0];
    if (c0) {
      if (dfs) c0->fix_edges(env, true);
      forest* c1 = edge.child[1];
      derivation_type const& d0 = get_child_best(*c0);
      if (d0 == NONE()) return;
      if (c1) {
        if (dfs) c1->fix_edges(env, true);
        derivation_type const& d1 = get_child_best(*c1);
        if (d1 != NONE()) {
          env.derivation_factory.fix_edge(edge.derivation, d0, get_child_best(*c1));
          return;
        }
      }
      env.derivation_factory.fix_edge(edge.derivation, d0);
    }
  }

  void fix_edges(Environment& env, bool dfs = true)  // only needed if you call sort() and the result may be
  // memo[0] != what you used for best subderivations for
  // that node
  {
    if (empty()) return;
    assert(!memo.empty() && !pq.empty());
    hyperedge const& best = top();
    if (!env.derivation_factory.needs_fixing(best.derivation)) return;
    // needs_fixing can mark corrected states so calling fix_edge(true) on every state isn't quadratic.
    assert(pq[0].derivation == memo[0]);
    for (typename pq_t::iterator i = pq.begin(), e = pq.end(); i != e; ++i) fix_hyperedge(env, *i, dfs);
    memo[0] = best.derivation;
  }

  template <class O>
  void print(O& o) const {
    print(o, 2);
  }

  // INVARIANT: pq[0] contains the last entry added to memo
  /// (this is true after sort() or first add_sorted())
  template <class O>
  void print(O& o, std::size_t nqueue) const {
    o << "{NODE @" << this << '[' << memo.size() << ']';
    std::size_t s = (std::size_t)memo_size();
    o << " #queued=" << pq_size();
    if (nqueue > pq.size()) nqueue = pq.size();
    for (std::size_t i = 0; i < nqueue; ++i) {
      o << "\n q[" << i << "]={{{";
      o << pq[i];
      o << "}}}";
    }
    if (s > (nqueue ? 0u : 0u)) {
      o << "\n first={{{";
      adl_print(o, first_best(), filter());  // o<< first_best();
      o << "}}}";
      if (s > 2) {
        o << "\n last={{{";
        adl_print(o, last_best(), filter());  // o<< last_best();
        o << "}}}";
      }
      // o << " pq=" << pq;
      // o<< pq; // " << memo=" << memo
    }
    o << '}';
  }

  /// if you have any state in your factory, assign to it here
  // TODO: non-static, please (or thread specific/dynamic scoped? passed as arguments)

  /// set this to true if you want negative cost cycles to throw rather than silently stop producing
  /// successors
  static bool is_null(derivation_type d) { return d == NONE(); }

  filter_type& filter() { return *this; }

  filter_type const& filter() const { return *this; }

  /// return the nth best (starting from 0) or NONE() (test with is_null(d)
  /// if the finite # of derivations in the forest is exhausted.
  /// IDEA: LAZY!!
  /// - only do the work of computing succesors to nth best when somebody ASKS
  /// for n+1thbest

  /// INVARIANT: pq[0] contains the last finally-computed memo entry. we pop it only when asked for successors
  /// to avoid generating 2nd bests everywhere if you only asked for 1-best at top

  /// (this is true after sort() or first add_sorted()) IF: a new n is asked
  /// for: must be 1 off the end of memo; push it as PENDING and go to work:
  /// {get succesors to pq[0] and heapify, storing memo[n]=top(). if no more
  /// left, memo[n]=NONE()} You're DONE when: pq is empty, or memo[n] = NONE()
  derivation_type get_best(Environment& env, lazy_kbest_index_type n) {
    EIFDBG(LAZYF, 2, KBESTINFOT("GET_BEST n=" << n << " node=" << *this); KBESTNESTT);
    if (n < memo.size()) {
      EIFDBG(LAZYF, 3,
             KBESTINFOT("existing " << this << "[n=" << n << "] = " << memo[n] << ", queue=" << *this));
      if (memo[n] == PENDING()) {
        KBESTERRORQ("lazy_forest::get_best",
                    "memo entry " << n << " for lazy_forest@0x" << (void*)this
                                  << " is pending - there must be a negative cost (or maybe 0-cost) cycle - "
                                     "returning NONE instead (this means that we don't generate any nbest "
                                     "above " << n << " for this node.");  //=" << memo[n-1]
        if (env.throw_on_cycle) throw lazy_derivation_cycle();
        memo[n] = NONE();
      }
      return memo[n];  // may be NONE
    } else {
      assertlvl(19, n == memo.size());
      assert(n > 0 && (pq.empty() || memo[n - 1] == pq[0].derivation));
      memo.push_back(PENDING());
      derivation_type& d = memo.back();
      for (;;) {
        if (pq.empty()) return (d = NONE());
        // assert(memo[n-1]==pq[0].derivation); // INVARIANT // no longer true with filtering
        d = next_best(env);
        EIFDBG(LAZYF, 2, KBESTINFOT(this << "[n=" << n << "] = " << d << ", queue=" << *this));
        if (d == NONE()) return d;
        if (filter().permit(d)) {
          EIFDBG(LAZYF, 2, KBESTINFOT("passed " << n << "th best for " << *this));
          ++env.stats.n_passed;
          return d;
        }  // else, try again:
        EIFDBG(LAZYF, 2, KBESTINFOT("filtered candidate " << n << "th best for " << *this));
        ++env.stats.n_filtered;
        d = PENDING();
      }
    }
  }
  /// returns last non-DONE derivation (one must exist!)
  derivation_type last_best() const {
    assertlvl(11, memo.size() && memo.front() != NONE());
    if (memo.back() != PENDING() && memo.back() != NONE()) return memo.back();
    assertlvl(11, memo.size() > 1);
    return *(memo.end() - 2);
  }
  bool empty() const { return !memo.size() || memo.front() == NONE() || memo.front() == PENDING(); }
  std::size_t pq_size() const { return pq.size(); }
  lazy_kbest_index_type memo_size() const {
    lazy_kbest_index_type r = (lazy_kbest_index_type)memo.size();
    for (;;) {
      if (r == 0) return 0;
      --r;
      if ((memo[r] != NONE() && memo[r] != PENDING())) return r + 1;
    }
  }

  /// returns best non-DONE derivation (one must exist!)
  derivation_type first_best() const {
    assertlvl(11, memo.size() && memo.front() != NONE() && memo.front() != PENDING());
    return memo.front();
  }

  /// Get next best derivation, or NONE if no more.
  //// INVARIANT: top() contains the next best entry
  /// PRECONDITION: don't call if pq is empty. (if !pq.empty()). memo ends with [old top derivation, PENDING].
  derivation_type next_best(Environment& env) {
    assertlvl(11, !pq.empty());

    hyperedge pending = top();  // creating a copy saves ugly complexities in trying to make pop_heap /
    // push_heap efficient ...
    EIFDBG(LAZYF, 1, KBESTINFOT("GENERATE SUCCESSORS FOR " << this << '[' << memo.size() << "] = " << pending));
    pop();  // since we made a copy already into pending...

    derivation_type old_parent
        = pending
              .derivation;  // remember this because we'll be destructively updating pending.derivation below
    //    assertlvl(19, memo.size()>=2 && memo.back() == PENDING()); // used to be true when not removing
    //    duplicates: && old_parent==memo[memo.size()-2]
    if (pending.child[0]) {  // increment first
      generate_successor_hyperedge(env, pending, old_parent, 0);
      if (pending.child[1]
          && pending.childbp[0] == 0) {  // increment second only if first is initial - one path to any (a, b)
        generate_successor_hyperedge(env, pending, old_parent, 1);
      }
    }
    if (pq.empty())
      return NONE();
    else {
      EIFDBG(LAZYF, 2, SHOWM2(LAZYF, "next_best", top().derivation, this));
      return top().derivation;
    }
  }

  // doesn't add to queue
  void set_first_best(Environment& env, derivation_type r) {
    assert(memo.empty());
    memo.push_back(r);
    if (!filter().permit(r))
      throw std::runtime_error("lazy_forest_kbest: the first-best derivation can never be filtered out");
    ++env.stats.n_passed;
  }

  /// may be followed by add() or add_sorted() equivalently
  void add_first_sorted(Environment& env, derivation_type r, forest* left = NULL, forest* right = NULL) {
    assert(pq.empty() && memo.empty());
    set_first_best(env, r);
    add(r, left, right);
    EIFDBG(LAZYF, 3, KBESTINFOT("add_first_sorted lazy-node=" << this << ": " << *this));
  }

  /// must be added from best to worst order ( r1 > r2 > r3 > ... )
  void add_sorted(Environment& env, derivation_type r, forest* left = NULL, forest* right = NULL) {
#ifndef NDEBUG
    if (!pq.empty()) {
      assert(call_derivation_better_than(pq[0].derivation, r));
      unsigned i = pq.size();
      assert(i > 0);
      unsigned p = (i - 1);
      assert(p >= 0);
      assert(call_derivation_better_than(pq[p].derivation, r));  // heap property
    }
#endif
    add(r, left, right);
    if (pq.size() == 1) set_first_best(env, r);
  }

  void reserve(std::size_t n) { pq.reserve(n); }

  /// may add in any order, but must call sort() before any get_best()
  void add(derivation_type r, forest* left = NULL, forest* right = NULL) {
    EIFDBG(LAZYF, 2, KBESTINFOT("add lazy-node=" << this << " derivation=" << r << " left=" << left
                                                 << " right=" << right));
    pq.push_back(hyperedge(r, left, right));
    EIFDBG(LAZYF, 3, KBESTINFOT("done (heap) " << r));
  }

  void sort(Environment& env, bool check_best_is_selfloop = false) {
    std::make_heap(pq.begin(), pq.end());
    finish_adding(env, check_best_is_selfloop);
    EIFDBG(LAZYF, 3, KBESTINFOT("sorted lazy-node=" << this << ": " << *this));
  }

  bool best_is_selfloop() const { return top().self_loop(this); }

  // note: if 2nd best is selfloop, you're screwed. violates heap rule temporarily; doing indefinitely would
  // require changing the comparison function. returns true if everything is ok after, false if we still have
  // a selfloop as first-best
  bool postpone_selfloop() {
    if (!best_is_selfloop()) return true;
    if (pq.size() == 1) return false;
    if (pq.size() > 2 && pq[1] < pq[2])  // STL heap=maxheap. 2 better than 1.
      std::swap(pq[0], pq[2]);
    else
      std::swap(pq[0], pq[1]);
    return !best_is_selfloop();
  }

  /// optional: if you call only add() on sorted, you must finish_adding().
  /// if you added unsorted, don't call this - call sort() instead
  void finish_adding(Environment& env, bool check_best_is_selfloop = false) {
    if (pq.empty())
      set_first_best(env, NONE());
    else if (check_best_is_selfloop && !postpone_selfloop())
      throw lazy_derivation_cycle();
    else
      set_first_best(env, top().derivation);
  }

  // note: postpone_selfloop() may make this not true.
  bool is_sorted() {
    return true;  // TODO: implement
  }

  // private:
  typedef std::vector<hyperedge> pq_t;
  typedef std::vector<derivation_type> memo_t;

  // MEMBERS:
  pq_t pq;  // INVARIANT: pq[0] contains the last entry added to memo
  memo_t memo;

  // try worsening ith (0=left, 1=right) child and adding to queue
  inline void generate_successor_hyperedge(Environment& env, hyperedge& pending, derivation_type old_parent,
                                           lazy_kbest_index_type i) {
    lazy_forest& child_node = *pending.child[i];
    lazy_kbest_index_type& child_i = pending.childbp[i];
    derivation_type old_child = child_node.memo[child_i];
    EIFDBG(LAZYF, 2,
           KBESTINFOT("generate_successor_hyperedge #" << i << " @" << this << ": "
                                                       << " old_parent=" << old_parent
                                                       << " old_child=" << old_child << " NODE=" << *this);
           KBESTNESTT);

    derivation_type new_child = (child_node.get_best(env, ++child_i));
    if (new_child != NONE()) {  // has child-succesor
      EIFDBG(LAZYF, 6, KBESTINFOT("HAVE CHILD SUCCESSOR for i=" << i << ": [" << pending.childbp[0] << ','
                                                                << pending.childbp[1] << "]");
             SHOWM7(LAZYF, "generator_successor-child-i", i, pending.childbp[0], pending.childbp[1],
                    old_parent, old_child, new_child, child_node));
      pending.derivation = env.derivation_factory.make_worse(old_parent, old_child, new_child, i);
      EIFDBG(LAZYF, 4, KBESTINFOT("new derivation: " << pending.derivation));
      push(pending);
    }
    --child_i;
    // KBESTINFOT("restored original i=" << i << ": [" << pending.childbp[0] << ',' << pending.childbp[1] <<
    // "]");
  }

  void push(const hyperedge& e) {
    pq.push_back(e);
    std::push_heap(pq.begin(), pq.end());
    // This algorithm puts the element at position end()-1 into what must be a pre-existing heap consisting of
    // all elements in the range [begin(), end()-1), with the result that all elements in the range [begin(),
    // end()) will form the new heap. Hence, before applying this algorithm, you should make sure you have a
    // heap in v, and then add the new element to the end of v via the push_back member function.
  }
  void pop() {
    std::pop_heap(pq.begin(), pq.end());
    // This algorithm exchanges the elements at begin() and end()-1, and then rebuilds the heap over the range
    // [begin(), end()-1). Note that the element at position end()-1, which is no longer part of the heap,
    // will nevertheless still be in the vector v, unless it is explicitly removed.
    pq.pop_back();
  }
  const hyperedge& top() const { return pq.front(); }
};

}  // ns

CLANG_DIAG_ON(unused-variable)

#endif
