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
/** \file

    .
*/

#ifndef GRAEHL_SHARED__LAZIER_FOREST_HPP
#define GRAEHL_SHARED__LAZIER_FOREST_HPP
#pragma once

#include <graehl/shared/lazy_forest_kbest.hpp>
#include <graehl/shared/pointer_int.hpp>
#include <graehl/shared/priority_queue.hpp>
#include <graehl/shared/small_vector.hpp>
#include <graehl/shared/int_hash_map.hpp>
#include <boost/pool/pool.hpp>

#include <graehl/shared/warning_compiler.h>
CLANG_DIAG_OFF(unused-variable)
namespace graehl {

template <class Derivation>
struct lazy_hyperedge {
  typedef lazy_hyperedge self_type;
  typedef Derivation derivation_type;
  /// antecedent subderivation forest (OR-nodes). if unary,
  /// child[1]==NULL. leaves have child[0]==NULL also.
  typedef void* forestptr;  // int_or_pointer - if int, needs DerivationFactory to build
  forestptr child[2];
  /// index into kth best for that child's OR-node: which of the possible
  /// children we chose (0=best-cost, 1 next-best ...)
  lazy_kbest_index_type childbp[2];
  /// the derivation that resulted from choosing the childbp[0]th out of
  /// child[0] and childbp[1]th out of child[1]
  derivation_type derivation;

  void set(derivation_type _derivation, forestptr c0 = 0, forestptr c1 = 0) {
    childbp[0] = childbp[1] = 0;
    derivation = _derivation;
    child[0] = c0;
    child[1] = c1;
  }
  void set(derivation_type _derivation, std::size_t state0) {
    childbp[0] = childbp[1] = 0;
    derivation = _derivation;
    set_pointer_int(child[0], state0);
    child[1] = 0;
  }
  void set(derivation_type _derivation, std::size_t state0, std::size_t state1) {
    childbp[0] = childbp[1] = 0;
    derivation = _derivation;
    set_pointer_int(child[0], state0);
    set_pointer_int(child[1], state1);
  }

  // NB: std::pop_heap puts largest element at top (max-heap)
  inline bool operator<(
      lazy_hyperedge const& o) const {  // true if o should dominate max-heap, i.e. o is better than us
    return call_derivation_better_than(o.derivation, derivation);
  }
  // means: this<o iff o better than this. good.

  template <class O>
  void print(O& o) const {
    o << "{lazy_hyperedge(";
    if (child[0]) {
      o << PointerInt(child[0]) << '[' << childbp[0] << ']';
      if (child[1]) o << "," << PointerInt(child[1]) << '[' << childbp[1] << ']';
    }
    o << ")=" << derivation;
    o << '}';
  }
  template <class C, class T>
  friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, lazy_hyperedge const& self) {
    self.print(o);
    return o;
  }
  /**
     only used by priority_queue.contains(lazy_hyperedge), which we don't
     call. so the fact that some child pointers might be expanded and some might
     not yet be isn't an issue (yet)
  */
  bool operator==(lazy_hyperedge const& o) const {
    if (o.child[0] != child[0]) return false;
    if (o.child[1] != child[1]) return false;
    if (o.childbp[0] != childbp[0]) return false;
    if (o.childbp[1] != childbp[1]) return false;
    if (o.derivation != derivation) return false;
    return true;
  }
};

template <class DerivationFactory, class FilterFactory = permissive_kbest_filter_factory>
struct lazier_forest;

template <class DerivationFactory>
struct lazier_forest_options {
  typedef typename DerivationFactory::derivation_type derivation_type;
  typedef lazy_hyperedge<derivation_type> hyperedge;
  typedef small_vector<hyperedge, 3, lazy_kbest_index_type> hyperedges_container;  // lazy_hyperedge is POD
  typedef priority_queue<hyperedges_container, 4> hyperedges_queue;

  typedef void* forestptr;

  lazier_forest_options(lazy_kbest_index_type pqReserve = 0) : throw_on_cycle(), pqReserve(pqReserve) {}

  DerivationFactory derivation;
  lazy_kbest_stats stats;
  bool throw_on_cycle;
  lazy_kbest_index_type pqReserve;
};

template <class DerivationFactory, class FilterFactory = permissive_kbest_filter_factory>
struct lazier_forest_filter_options {
  typedef lazier_forest_options<DerivationFactory> Base;
  typedef DerivationFactory derivation_factory_type;

  typedef FilterFactory filter_factory_type;
  FilterFactory filter;

  boost::pool<> forestpool;
  int_ptr_map forests;

  typedef lazier_forest<DerivationFactory, FilterFactory> forest;

  typedef void* ptr_or_int;
  forest& expand(ptr_or_int& p) {
    if (is_pointer(p)) return *(forest*)p;
    std::size_t forestid = pointer_int(p);
    ptr_or_int& fp = forests[forestid];
    if (fp) return *(forest*)(p = fp);
    forest* f = (forest*)(f = forestpool.malloc());
    new (f) forest();
    if (this->derivation.expand(forestid, f->pq))
      f->finish_adding(*this);
    else
      f->sort(*this);
    p = f;
    return *f;
  }

  lazier_forest_filter_options(std::size_t reserveForestIdMap = 1000000, lazy_kbest_index_type pqReserve = 0,
                               std::size_t forestSz = sizeof(forest))
      : Base(pqReserve), forestpool(forestSz), forests(reserveForestIdMap) {}

  void clear_stats() { this->stats.clear(FilterFactory::filter_type::trivial); }
};

/**
   this is like lazy_forest except you don't build in advance all the lazy_forest nodes. instead your
   DerivationFactory also provides:

   struct MyDerivFactory {

   typedef char derivation_type; // probably not char :)

   /// add lazy_hyperedge with if return is true, then they were added sorted.
   bool expand(std::size_t stateid, typename lazier_forest_options<MyDerivFactory>::lazy_hyperedges_queue
   &edges);

   };

*/
template <class DerivationFactory, class FilterFactory>
struct lazier_forest
    : FilterFactory::filter_type  // empty base class opt. - may have state e.g. hash of seen strings or trees
      {
  typedef DerivationFactory derivation_factory_type;
  typedef FilterFactory filter_factory_type;
  typedef lazier_forest_filter_options<derivation_factory_type, filter_factory_type> options;

  typedef typename options::hyperedge hyperedge;
  typedef void* forestptr;
  typedef typename filter_factory_type::filter_type filter_type;
  typedef lazy_forest<derivation_factory_type, filter_factory_type> forest;
  typedef forest self_type;
  typedef typename derivation_factory_type::vertex_descriptor vertex_descriptor;

  explicit lazier_forest(options const& opt) : filter_type(opt.filter.filter_init()), pq(opt.pqReserve) {}

  typedef typename derivation_factory_type::derivation_type derivation_type;
  static inline derivation_type NONE() {
    return (derivation_type)DerivationFactory::NONE();  // nonmember for static is_null
  }
  static inline derivation_type PENDING() { return (derivation_type)DerivationFactory::PENDING(); }
  /// bool Visitor(derivation, ith) - if returns false, then stop early.
  /// otherwise stop after generating up to k (up to as many as exist in
  /// forest)
  template <class Visitor>
  lazy_kbest_stats enumerate_kbest(options& opt, lazy_kbest_index_type k, Visitor visit = Visitor()) {
    EIFDBG(LAZYF, 1, KBESTINFOT("COMPUTING BEST " << k << " for node " << *this); KBESTNESTT;);

    opt.clear_stats();
    for (lazy_kbest_index_type i = 0; i < k; ++i) {
      EIFDBG(LAZYF, 2, SHOWM2(LAZYF, "enumerate_kbest-pre", i, *this));
      derivation_type ith_best = get_best(opt, i);
      if (ith_best == NONE()) break;
      if (!visit(ith_best, i)) break;
    }
    return opt.stats;
  }

  void swap(self_type& o) {
    pq.swap(o.pq);
    memo.swap(o.memo);
  }

  // TODO: faster heap operations if we put handles to hyperedges on heap instead of copying? boost object
  // pool?

  static inline derivation_type get_child_best(forest const& f) {
    return f.pq.empty() ? NONE() : f.pq.top().derivation;
  }

  // TODO: fix_edges (force memo[0] to agree with pq[0] e.g. ties+sort

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
  derivation_type get_best(options& opt, lazy_kbest_index_type n) {
    EIFDBG(LAZYF, 2, KBESTINFOT("GET_BEST n=" << n << " node=" << *this); KBESTNESTT);
    if (n < memo.size()) {
      EIFDBG(LAZYF, 3,
             KBESTINFOT("existing " << this << "[n=" << n << "] = " << memo[n] << ", queue=" << *this));
      if (memo[n] == PENDING()) {
        KBESTERRORQ("lazier_forest::get_best",
                    "memo entry " << n << " for lazier_forest@0x" << (void*)this
                                  << " is pending - there must be a negative cost (or maybe 0-cost) cycle - "
                                     "returning NONE instead (this means that we don't generate any nbest "
                                     "above " << n << " for this node.");  //=" << memo[n-1]
        if (opt.throw_on_cycle) throw lazy_derivation_cycle();
        memo[n] = NONE();
      }
      return memo[n];  // may be NONE
    } else {
      assertlvl(19, n == memo.size());
      assert(n > 0 && (pq.empty() || memo[n - 1] == pq.top().derivation));
      memo.push_back(PENDING());
      derivation_type& d = memo.back();
      for (;;) {
        if (pq.empty()) return (d = NONE());
        d = next_best(opt);
        EIFDBG(LAZYF, 2, KBESTINFOT(this << "[n=" << n << "] = " << d << ", queue=" << *this));
        if (d == NONE()) return d;
        if (filter().permit(d)) {
          EIFDBG(LAZYF, 2, KBESTINFOT("passed " << n << "th best for " << *this));
          ++opt.stats.n_passed;
          return d;
        }  // else, try again:
        EIFDBG(LAZYF, 2, KBESTINFOT("filtered candidate " << n << "th best for " << *this));
        ++opt.stats.n_filtered;
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
  /// PRECONDITION: !pq.empty()
  derivation_type next_best(options& opt) {
    assertlvl(11, !pq.empty());

    hyperedge pending = top();  // creating a copy saves ugly complexities in trying to make pop_heap /
                                // push_heap efficient ...
    EIFDBG(LAZYF, 1,
           KBESTINFOT("GENERATE SUCCESSORS FOR " << this << '[' << memo.size() << "] = " << pending));
    pop();  // since we made a copy already into pending...

    derivation_type old_parent
        = pending
              .derivation;  // remember this because we'll be destructively updating pending.derivation below
    //    assertlvl(19, memo.size()>=2 && memo.back() == PENDING()); // used to be true when not removing
    //    duplicates: && old_parent==memo[memo.size()-2]
    if (pending.child[0]) {  // increment first
      generate_successor_hyperedge(opt, pending, old_parent, 0);
      if (pending.child[1]
          && pending.childbp[0] == 0) {  // increment second only if first is initial - one path to any (a, b)
        generate_successor_hyperedge(opt, pending, old_parent, 1);
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
  void set_first_best(options& opt, derivation_type r) {
    assert(memo.empty());
    memo.push_back(r);
    if (r == NONE()) return;
    if (!filter().permit(r))
      throw std::runtime_error("lazier_forest_kbest: the first-best derivation can never be filtered out");
    ++opt.stats.n_passed;
  }

  void reserve(std::size_t n) { pq.reserve(n); }

  void sort(options& opt) {
    pq.heapify();
    finish_adding(opt);
    EIFDBG(LAZYF, 3, KBESTINFOT("sorted lazy-node=" << this << ": " << *this));
  }

  // TODO: check for self-loop? postpone_selfloop violating heap property?

  /// optional: if you call only add() on sorted, you must finish_adding().
  /// if you added unsorted, don't call this - call sort() instead
  void finish_adding(options& opt) {
    if (pq.empty())
      set_first_best(opt, NONE());
    else
      set_first_best(opt, top().derivation);
  }

  // note: postpone_selfloop() may make this not true.
  bool is_sorted() {
    return true;  // FIXME: implement
  }

  // private:
  typedef std::vector<derivation_type> memo_t;  // derivation_type may not be POD
  typedef typename options::hyperedges_queue hyperedges_queue;

  // MEMBERS:
  hyperedges_queue pq;  // INVARIANT: pq.top() contains the last entry added to memo
  memo_t memo;

  // try worsening ith (0=left, 1=right) child and adding to queue
  inline void generate_successor_hyperedge(options& opt, hyperedge& pending, derivation_type old_parent,
                                           lazy_kbest_index_type i) {
    lazier_forest& child_node = opt.expand(pending.child[i]);
    lazy_kbest_index_type& child_i = pending.childbp[i];
    derivation_type old_child = child_node.memo[child_i];
    EIFDBG(LAZYF, 2,
           KBESTINFOT("generate_successor_hyperedge #" << i << " @" << this << ": "
                                                       << " old_parent=" << old_parent
                                                       << " old_child=" << old_child << " NODE=" << *this);
           KBESTNESTT);

    derivation_type new_child = (child_node.get_best(++child_i));
    if (new_child != NONE()) {  // has child-succesor
      EIFDBG(LAZYF, 6, KBESTINFOT("HAVE CHILD SUCCESSOR for i=" << i << ": [" << pending.childbp[0] << ','
                                                                << pending.childbp[1] << "]");
             SHOWM7(LAZYF, "generator_successor-child-i", i, pending.childbp[0], pending.childbp[1],
                    old_parent, old_child, new_child, child_node));
      pending.derivation = opt.derivation.make_worse(old_parent, old_child, new_child, i);
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


}

CLANG_DIAG_ON(unused-variable)

#endif
