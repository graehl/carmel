#ifndef GRAEHL__SHARED__LAZY_FOREST_KBEST_HPP
#define GRAEHL__SHARED__LAZY_FOREST_KBEST_HPP

// you may override this with a fully namespace qualified type - but be careful to do so consistently before every inclusion!
// in practice unsigned would be great. who visits > 4billion best?
#ifndef LAZY_FOREST_KBEST_SIZE
# define LAZY_FOREST_KBEST_SIZE std::size_t
#endif
//TODO: cycles - if you found a pending, then try not queueing successors until you're added to memo table. but MAYBE our successors first approach makes us handle negative cost improvements more nicely? not sure. if not, then always queue successors afterwards.

//FIXME: see postpone_selfloop for weakness in current cycle handling - apparently want a non-cycle 2nd best, then it's sort of ok?

//FIXME: uses static (global) state - so only one lazy kbest can be in progress. can't imagine why that would be a problem, but watch out!

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
  derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child, lazy_kbest_index_type changed_child_index)
  {
  return new Result(prototype,old_child,new_child,changed_child_index);
  }
  void fix_edge(derivation_type &wrong_1best,derivation_type correct_unary_best); // repair errors in non-sorted 1-best derivation
  void fix_edge(derivation_type &wrong_1best,derivation_type correct_left_best,derivation_type correct_right_best); // repair errors in non-sorted 1-best derivation
  };

  // you also need operator == (used only for assertions)

  bool derivation_better_than(derivation_type a,derivation_type b);

  or

  // Result(a) < Result(b) iff a is better than b.

  or specialize in ns graehl:

  namespace graehl {
  template<>
  struct lazy_kbest_derivation_traits<Deriv> {
  static inline bool better_than(const Deriv &candidate,const Deriv &than)
  {
  return candidate<than;
  }
  {

  then build a lazy_forest<Factory> binary hypergraph
*/

//TODO: use d_ary_heap.hpp (faster than binary)

///\{

#include <graehl/shared/os.hpp>
#ifdef NDEBUG
# define LAZYF(x)
#else
# define LAZYF(x) x
#endif
DECLARE_DBG_LEVEL(LAZYF)

#ifdef SAMPLE
# undef SAMPLE
# define LAZY_FOREST_EXAMPLES
# define LAZY_FOREST_SAMPLE
#endif

# include <graehl/shared/containers.hpp>
# include <graehl/shared/indent_level.hpp>
namespace graehl {
struct lazy_forest_indent_tag {};
# define LAZY_FOREST_KBEST_INDENT SCOPED_INDENT(lazy_forest_indent_tag)
}

#ifndef LAZY_FOREST_KBEST_MSG
# define LAZY_FOREST_KBEST_MSG(msg) SHOWP(LAZYF,LAZY_FOREST_KBEST_INDENT<<msg)
#endif

#include <graehl/shared/ifdbg.hpp>
#include <graehl/shared/show.hpp>
#include <boost/noncopyable.hpp>
#include <graehl/shared/percent.hpp>
#include <graehl/shared/assertlvl.hpp>


#ifdef GRAEHL_TEST
# include <graehl/shared/test.hpp>
# define LAZY_FOREST_EXAMPLES
#endif

#if defined(LAZY_FOREST_EXAMPLES)
# if USE_DEBUGPRINT
#  include <graehl/shared/info_debug.hpp>
//# include "default_print.hpp"
//FIXME: doesn't work
#  include <graehl/shared/debugprint.hpp>
# endif
# include <iostream>
# include <string>
# include <sstream>
# include <cmath>
#endif

#ifdef LAZY_FOREST_KBEST_MSG
# define KBESTINFOT(x) LAZY_FOREST_KBEST_MSG(x<<'\n')
#if defined(NESTT)
# define KBESTNESTT NESTT
#else
#if _MSC_VER
# define KBESTNESTT
#else
# define KBESTNESTT SCOPED_INDENT_NEST(lazy_forest_indent_tag)
#endif
#endif
#else
# define KBESTINFOT(x)
# define KBESTNESTT
#endif

#ifndef ERRORQ
# include <iostream>
# define KBESTERRORQ(x,y) do{ std::cerr << "\n" << x << ": "<<y<<std::endl; }while(0)
#else
# define KBESTERRORQ(x,y) ERRORQ(x,y)
#endif

#include <cstddef>
#include <vector>
#include <stdexcept>
#include <algorithm>
#ifdef __GNUC__
#include <ext/algorithm> // is_sorted
#endif

namespace graehl {

typedef LAZY_FOREST_KBEST_SIZE lazy_kbest_index_type;

inline std::ostream & operator <<(std::ostream &o,lazy_kbest_index_type bp[2])
{
  return o << '[' << bp[0] << ','<<bp[1]<<']';
}

/*
  template <class Deriv> inline
  bool derivation_better_than(const Deriv &me,const Deriv &than)
  {
  return me > than;
  }
*/

// you should probably only override (i.e. specialize) this... not sure how ADL will work. default is smaller (e.g. lower cost) is better.
template <class Deriv>
struct lazy_kbest_derivation_traits
{
  // Deriv(a) < Deriv(b) iff b is better than a.
  static inline bool better_than(const Deriv &candidate,const Deriv &than)
  {
    return candidate < than;
  }
};

/// but if you overload this, then you don't need to specialize above. this is what we actually call
template <class Deriv> inline
bool derivation_better_than(const Deriv &candidate,const Deriv &than)
{
  return lazy_kbest_derivation_traits<Deriv>::better_than(candidate,than);
}

/// but if you overload this, then you don't need to specialize above. this is what we actually call
template <class Deriv> inline
bool call_derivation_better_than(const Deriv &candidate,const Deriv &than)
{
  return derivation_better_than(candidate,than); //ADL here
}

struct lazy_derivation_cycle : public std::runtime_error
{
  lazy_derivation_cycle() : std::runtime_error("lazy_forest::get_best tried to compute a derivation that eventually depended on itself - probable cause: negative cost cycle") {}
};


/**
   Note: unfortunately, this entire code does not allow for multiple concurrent jobs (per type of factory), since there is a static set_derivation_factory. Change would be simple but consume a little extra memory (at each kbest node?)

   build a copy of your (at most binary) derivation forest, then query its root for the 1st, 2nd, ... best
   <code>
   struct DerivationFactory
   {


   /// can override derivation_better_than(derivation_type,derivation_type).
   /// derivation_type should be a lightweight (value) object
   /// if LAZY_FOREST_KBEST_MSG debug prints are enabled, must also have o << deriv
   typedef Result *derivation_type;

   friend bool derivation_better_than(derivation_type a,derivation_type b); // override this or lazy_kbest_derivation_traits::better_than. default is a>b

   /// special derivation values (not used for normal derivations) (may be of different type but convertible to derivation_type)
   static derivation_type PENDING();
   static derivation_type NONE();
/// derivation_type must support: initialization by (copy), assignment, ==, !=

/// take an originally better (or best) derivation, substituting for the changed_child_index-th child new_child for old_child (cost difference should usually be computed as (cost(new_child) - cost (old_child)))
derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child, lazy_kbest_index_type changed_child_index)
{
return new Result(prototype,old_child,new_child,changed_child_index);
}
};
</code>
**/
  // TODO: implement unique visitor of all the lazykbest subresults (hash by pointer to derivation?)

struct none_t {};
struct pending_t {};

struct dummy_init_type
{
  template <class Init>
  dummy_init_type(Init const& g) {}
};


// your factory may inherit form this - the do-nothing fix-best should suffice if you're using add_sorted only.
struct lazy_kbest_derivation_factory_base
{
  template <class A>
  void fix_edge(A &a,A const& unary) {}
  template <class A>
  void fix_edge(A &a,A const& left,A const& right) {}
  template <class A>
  bool needs_fixing(A &a) { return false; }
};


struct permissive_kbest_filter
{
  enum { trivial=1 }; // avoids printing stats on % filtered
  /*
    typedef some_stateful_derivation_predicate filter_type;
    some_type filter_init(); /// used like: typename factory_type::filter_type per_node_filter(factory.filter_init());
  */
  struct dummy_init_type
  {
    template <class Init>
    dummy_init_type(Init const& g) {}
  };

  typedef dummy_init_type init_type;
  permissive_kbest_filter() {}
  permissive_kbest_filter(init_type const& g) {}

  template <class E>
  bool permit(E const& e) const
  { return true; }

  template <class O,class E>
  friend inline void print(O &o,E const& e,permissive_kbest_filter const&)
  { o << e; }

};


struct permissive_kbest_filter_factory
{
  typedef permissive_kbest_filter filter_type;
  bool filter_init() const // gets passed to constructor for filter_type
  {
    return true;
  }
};

template <class filter>
struct default_filter_factory
{
  typedef filter filter_type;
  filter filter_init() const
  {
    return filter();
  }
};

// if you defined a filter which is copyable and supply an empty-state prototype to the factory:
template <class filter>
struct copy_filter_factory
{
  typedef filter filter_type;
  filter f;
  copy_filter_factory() {}
  copy_filter_factory(filter const& f) : f(f) {}
  copy_filter_factory(copy_filter_factory const& o) : f(o.f) {}
  filter const& filter_init() const
  {
    return f;
  }
};

//a system that passes in projections of children for optimization (maybe avoiding quadratic in sz(deriv) work). but hash key len will grow anyway.
template <class Deriv,class Projection,class MapSelect=UmapS>
struct projected_duplicate_filter : Projection
{
  typedef projected_duplicate_filter<Deriv,Projection,MapSelect> self_type;
  enum { trivial=0 };
  projected_duplicate_filter(projected_duplicate_filter const& o) : Projection(o),maxreps(o.maxreps) {}
  projected_duplicate_filter(unsigned maxreps=0,Projection const& projection=Projection())
    : Projection(projection), maxreps(maxreps) {}
  unsigned maxreps;
  typedef Deriv derivation_type;
  typedef typename Projection::result_type projected_type;
  typedef typename MapSelect::template map<projected_type,unsigned>::type projecteds_type;
  projecteds_type projecteds;
  bool permit(derivation_type const& d)
  {
    if (!maxreps) return true;
    projected_type const& projected=Projection::operator()(d);
    unsigned n=++projecteds[projected];
    EIFDBG(LAZYF,3,
           std::ostringstream projstr;
           print_projection(projstr,d);
           SHOWM3(LAZYF,"dup-filter permit?",projstr.str(),n,maxreps)
      );
    return n<=maxreps;
  }
  template <class O>
  void print_projection(O &o,derivation_type const& d) const
  {
    Projection::print_projection(o,d);
  }
/*  template <class O>
  void print(O &o,derivation_type const& d,projected_type const& proj)
  {
    o<<"projection for "<<&d<<": "<<&proj;
    }*/


  template <class O>
  friend inline void print(O &o,derivation_type const& d,self_type const& s)
  { s.print_projection(o,d); }
};


struct lazy_kbest_stats
{
  typedef std::size_t count_t;
  count_t n_passed;
  count_t n_filtered;
  bool trivial_filter;
  void clear(bool trivial_filt=false)
  {
    trivial_filter=trivial_filt;
    n_passed=n_filtered=0;
  }
  count_t n_total() const
  {
    return n_passed+n_filtered;
  }
  lazy_kbest_stats()
  { clear(); }

  template <class C, class T>
  void print(std::basic_ostream<C,T> &o) const
  {
    if (trivial_filter) {
      assert(!n_filtered);
      o<<"[Lazy kbest "<<n_passed<<" derivations found]";
      return;
    }
    o << "[Lazy kbest uniqueness-filtered "<<n_filtered<<" of "<<n_total()<<" derivations, leaving "
      << n_passed<<", or "<<graehl::percent<5>((double)n_passed,(double)n_total())<<"]";
  }
  typedef lazy_kbest_stats self_type;
};

template <class C, class T>
std::basic_ostream<C,T>&
operator<<(std::basic_ostream<C,T>& os, lazy_kbest_stats const& kb)
{
  kb.print(os);
  return os;
}

namespace {
template <class A,class B,class C>
void adl_print(A &a,B const& b,C const& c) {
  print(a,b,c);
}
template <class A,class B,class C,class D>
void adl_print(A &a,B const& b,C const& c,D const& d) {
  print(a,b,c,d);
}
}

// i've left this copyable even though you should for efficiency's sake only copy empty things, e.g. if you want to compile a vector of them that's fine
template <class DerivationFactory,class FilterFactory=permissive_kbest_filter_factory>
class lazy_forest
  : public FilterFactory::filter_type // empty base class opt. - may have state e.g. hash of seen strings or trees
{

public:

  typedef DerivationFactory derivation_factory_type;
  typedef FilterFactory filter_factory_type;
  typedef typename filter_factory_type::filter_type filter_type;
  typedef lazy_forest<derivation_factory_type,filter_factory_type> forest;
  typedef forest self_type;


  lazy_forest() : filter_type(filter_factory.filter_init()) {

  }

  typedef typename derivation_factory_type::derivation_type derivation_type;
  typedef derivation_factory_type D;
  static inline derivation_type NONE()
  {
    return (derivation_type)DerivationFactory::NONE(); // nonmember for static is_null
    //derivation_factory.NONE();
  }
  derivation_type PENDING() const
  {
    return (derivation_type)derivation_factory.PENDING();
  }

  void clear_stats()
  {
    stats.clear(filter_type::trivial);
  }

  /// bool Visitor(derivation,ith) - if returns false, then stop early.
  /// otherwise stop after generating up to k (up to as many as exist in
  /// forest)
  template <class Visitor>
  lazy_kbest_stats enumerate_kbest(lazy_kbest_index_type k,Visitor visit=Visitor()) {
    EIFDBG(LAZYF,1,KBESTINFOT("COMPUTING BEST " << k << " for node " << *this);KBESTNESTT;);
    clear_stats();
    for (lazy_kbest_index_type i=0;i<k;++i) {
      EIFDBG(LAZYF,2,SHOWM2(LAZYF,"enumerate_kbest-pre",i,*this));
      derivation_type ith_best=get_best(i);
      if (ith_best == NONE()) break;
      if (!visit(ith_best,i)) break;
    }
    return stats;
  }


  void swap(self_type &o)
  {
    pq.swap(o.pq);
    memo.swap(o.memo);
  }

  //FIXME: faster heap operations if we put handles to hyperedges on heap instead of copying? boost object pool?
  struct hyperedge {
    typedef hyperedge self_type;
    /// antecedent subderivation forest (OR-nodes). if unary,
    /// child[1]==NULL. leaves have child[0]==NULL also.
    forest *child[2];
    /// index into kth best for that child's OR-node: which of the possible
    /// children we chose (0=best-cost, 1 next-best ...)
    lazy_kbest_index_type childbp[2];
    /// the derivation that resulted from choosing the childbp[0]th out of
    /// child[0] and childbp[1]th out of child[1]
    derivation_type derivation;

    // returns NULL if not unary
    forest *unary_child() const
    {
      if (!child[1]) return NULL;
      return child[0];
    }

    hyperedge(derivation_type _derivation,forest *c0,forest *c1)
    {
      set(_derivation,c0,c1);
    }
    void set(derivation_type _derivation,forest *c0,forest *c1) {
      childbp[0]=childbp[1]=0;
      child[0]=c0;
      child[1]=c1;
      derivation=_derivation;
    }
    //NB: std::pop_heap puts largest element at top (max-heap)
    inline bool operator <(const hyperedge &o) const { // true if o should dominate max-heap, i.e. o is better than us
      return call_derivation_better_than(o.derivation,derivation);
    }
    // means: this<o iff o better than this. good.

    template <class O>
    void print(O &o) const {
      o << "{hyperedge(";
      if (child[0]) {
        o << child[0] << '[' << childbp[0] << ']';
        if (child[1])
          o << "," << child[1] << '[' << childbp[1] << ']';
      }
      o << ")=" << derivation;
      o << '}';
    }
    template <class C,class T>
    friend std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T> &o, hyperedge const& self)
    { self.print(o); return o; }
    bool operator==(hyperedge const& o) const
    {
      if (o.child[0]!=child[0]) return false;
      if (o.child[1]!=child[1]) return false;
      if (o.childbp[0]!=childbp[0]) return false;
      if (o.childbp[1]!=childbp[1]) return false;
      if (o.derivation!=derivation) return false;
      return true;
    }
  };

  static inline derivation_type get_child_best(forest const& f)
  {
    return f.pq.empty() ? NONE() : f.pq[0].derivation;
  }

  void fix_hyperedge(hyperedge &edge,bool dfs=true) {
    forest *c0=edge.child[0];
    if (c0) {
      if (dfs) c0->fix_edges(true);
      forest *c1=edge.child[1];
      derivation_type const& d0=get_child_best(*c0);
      if (d0==NONE()) return;
      if (c1) {
        if (dfs) c1->fix_edges(true);
        derivation_type const& d1=get_child_best(*c1);
        if (d1!=NONE()) {
          derivation_factory.fix_edge(edge.derivation,d0,get_child_best(*c1));
          return;
        }
      }
      derivation_factory.fix_edge(edge.derivation,d0);
    }
  }

  void fix_edges(bool dfs=true) // only needed if you call sort() and the result may be memo[0] != what you used for best subderivations for that node
  {
    if (empty()) return;
    assert(!memo.empty()&&!pq.empty());
    hyperedge const& best=top();
    if (!derivation_factory.needs_fixing(best.derivation))
      return;
     // needs_fixing can mark corrected states so calling fix_edge(true) on every state isn't quadratic.
    assert(pq[0].derivation==memo[0]);
    for (typename pq_t::iterator i=pq.begin(),e=pq.end();i!=e;++i)
      fix_hyperedge(*i,dfs);
    memo[0]=best.derivation;
  }

  template <class O>
  void print(O &o) const
  {
    print(o,2);
  }

  //INVARIANT: pq[0] contains the last entry added to memo
  /// (this is true after sort() or first add_sorted())
  template <class O>
  void print(O &o,std::size_t nqueue) const
  {
    o << "{NODE @" << this << '[' << memo.size() << ']';
    std::size_t s=(std::size_t)memo_size();
    o << " #queued="<<pq_size();
    if (nqueue>pq.size())
      nqueue=pq.size();
    for (std::size_t i=0;i<nqueue;++i) {
      o<<"\n q["<<i<<"]={{{";
      o<<pq[i];
      o<<"}}}";
    }
    if (s>(nqueue?0u:0u)) {
      o << "\n first={{{";
      adl_print(o,first_best(),filter()); //o<< first_best();
      o<< "}}}";
      if (s>2) {
        o << "\n last={{{";
        adl_print(o,last_best(),filter());// o<< last_best();
        o<< "}}}";
      }
// o << " pq=" << pq;
// o<< pq; // " << memo=" << memo
    }
    o << '}';
  }

  static void set_derivation_factory(derivation_factory_type const &df)
  {
    derivation_factory=df;
  }
  static void set_filter_factory(filter_factory_type const &f)
  {
    filter_factory=f;
  }

  /// if you have any state in your factory, assign to it here
  //TODO: non-static, please (or thread specific/dynamic scoped? passed as arguments)
  static derivation_factory_type derivation_factory;
  static filter_factory_type filter_factory;
  static lazy_kbest_stats stats;

  /// set this to true if you want negative cost cycles to throw rather than silently stop producing successors
  static bool throw_on_cycle;

  static bool is_null(derivation_type d)
  {
    return d==NONE();
  }

  filter_type & filter()
  { return *this; }

  filter_type const& filter() const
  { return *this; }

  /// return the nth best (starting from 0) or NONE() (test with is_null(d)
  /// if the finite # of derivations in the forest is exhausted.
  /// IDEA: LAZY!!
  /// - only do the work of computing succesors to nth best when somebody ASKS
  /// for n+1thbest

  /// INVARIANT: pq[0] contains the last finally-computed memo entry. we pop it only when asked for successors to avoid generating 2nd bests everywhere if you only asked for 1-best at top

  /// (this is true after sort() or first add_sorted()) IF: a new n is asked
  /// for: must be 1 off the end of memo; push it as PENDING and go to work:
  /// {get succesors to pq[0] and heapify, storing memo[n]=top(). if no more
  /// left, memo[n]=NONE()} You're DONE when: pq is empty, or memo[n] = NONE()
  derivation_type get_best(lazy_kbest_index_type n) {
    EIFDBG(LAZYF,1,KBESTINFOT("GET_BEST n=" << n << " node=" << *this);
           KBESTNESTT);
    if (n < memo.size()) {
      EIFDBG(LAZYF,3,KBESTINFOT("existing "<<this<<"[n="<<n<<"] = "<<memo[n]<<", queue="<<*this));
      if (memo[n] == PENDING()) {
        KBESTERRORQ("lazy_forest::get_best","memo entry " << n << " for lazy_forest@0x" << (void*)this<<" is pending - there must be a negative cost (or maybe 0-cost) cycle - returning NONE instead (this means that we don't generate any nbest above " << n << " for this node."); //=" << memo[n-1]
        if (throw_on_cycle)
          throw lazy_derivation_cycle();
        memo[n] = NONE();
      }
      return memo[n]; // may be NONE
    } else {
      assertlvl(19,n==memo.size());
      assert(n>0 && (pq.empty() || memo[n-1]==pq[0].derivation));
      memo.push_back(PENDING());
      derivation_type &d=memo.back();
      for(;;) {
        if (pq.empty())
          return (d=NONE());
        // assert(memo[n-1]==pq[0].derivation); // INVARIANT // no longer true with filtering
        d=next_best();
        EIFDBG(LAZYF,2,KBESTINFOT(this<<"[n="<<n<<"] = "<<d<<", queue="<<*this));
        if (d==NONE())
          return d;
        if (filter().permit(d)) {
          EIFDBG(LAZYF,2,KBESTINFOT("passed "<<n<<"th best for "<<*this));
          ++stats.n_passed;
          return d;
        } // else, try again:
        EIFDBG(LAZYF,2,KBESTINFOT("filtered candidate "<<n<<"th best for "<<*this));
        ++stats.n_filtered;
        d=PENDING();
      }
    }
  }
  /// returns last non-DONE derivation (one must exist!)
  derivation_type last_best() const {
    assertlvl(11,memo.size() && memo.front() != NONE());
    if (memo.back() != PENDING() && memo.back() != NONE())
      return memo.back();
    assertlvl(11,memo.size()>1);
    return *(memo.end()-2);
  }
  bool empty() const
  {
    return !memo.size() || memo.front()==NONE() || memo.front()==PENDING();
  }
  std::size_t pq_size() const
  {
    return pq.size();
  }
  lazy_kbest_index_type memo_size() const
  {
    lazy_kbest_index_type r=(lazy_kbest_index_type)memo.size();
    for (;;) {
      if (r==0) return 0;
      --r;
      if ((memo[r]!=NONE() && memo[r]!=PENDING()))
        return r+1;
    }
  }

  /// returns best non-DONE derivation (one must exist!)
  derivation_type first_best() const {
    assertlvl(11,memo.size() && memo.front() != NONE() && memo.front() != PENDING());
    return memo.front();
  }

  /// Get next best derivation, or NONE if no more.
  //// INVARIANT: top() contains the next best entry
  /// PRECONDITION: don't call if pq is empty. (if !pq.empty()). memo ends with [old top derivation,PENDING].
  derivation_type next_best() {
    assertlvl(11,!pq.empty());

    hyperedge pending=top(); // creating a copy saves ugly complexities in trying to make pop_heap / push_heap efficient ...
    EIFDBG(LAZYF,1,KBESTINFOT("GENERATE SUCCESSORS FOR "<<this<<'['<<memo.size()<<"] = "<<pending));
    pop(); // since we made a copy already into pending...

    derivation_type old_parent=pending.derivation; // remember this because we'll be destructively updating pending.derivation below
//    assertlvl(19,memo.size()>=2 && memo.back() == PENDING()); // used to be true when not removing duplicates: && old_parent==memo[memo.size()-2]
    if (pending.child[0]) { // increment first
      generate_successor_hyperedge(pending,old_parent,0);
      if (pending.child[1] && pending.childbp[0]==0) { // increment second only if first is initial - one path to any (a,b)
        generate_successor_hyperedge(pending,old_parent,1);
      }
    }
    if (pq.empty())
      return NONE();
    else {
      EIFDBG(LAZYF,2,SHOWM2(LAZYF,"next_best",top().derivation,this));
      return top().derivation;
    }
  }

  // doesn't add to queue
  void set_first_best(derivation_type r)
  {
    assert(memo.empty());
    memo.push_back(r);
    if (!filter().permit(r))
      throw std::runtime_error("lazy_forest_kbest: the first-best derivation can never be filtered out");
    ++stats.n_passed;
  }

  /// may be followed by add() or add_sorted() equivalently
  void add_first_sorted(derivation_type r,forest *left=NULL,forest *right=NULL)
  {
    assert(pq.empty() && memo.empty());
    set_first_best(r);
    add(r,left,right);
    EIFDBG(LAZYF,3,KBESTINFOT("add_first_sorted lazy-node="<<this<<": "<<*this));
  }

  /// must be added from best to worst order ( r1 > r2 > r3 > ... )
  void add_sorted(derivation_type r,forest *left=NULL,forest *right=NULL) {
#ifndef NDEBUG
    if (!pq.empty()) {
      assert(call_derivation_better_than(pq[0].derivation,r));
      unsigned i=pq.size();
      assert(i>0);
      unsigned p=(i-1);
      assert(p>=0);
      assert(call_derivation_better_than(pq[p].derivation,r)); // heap property
    }
#endif
    add(r,left,right);
    if (pq.size()==1)
      set_first_best(r);
  }

  void reserve(std::size_t n)
  {
    pq.reserve(n);
  }

  /// may add in any order, but must call sort() before any get_best()
  void add(derivation_type r,forest *left=NULL,forest *right=NULL)
  {
    EIFDBG(LAZYF,2,KBESTINFOT("add lazy-node=" << this << " derivation=" << r << " left=" << left << " right=" << right));
    pq.push_back(hyperedge(r,left,right));
    EIFDBG(LAZYF,3,KBESTINFOT("done (heap) " << r));
  }

  void sort(bool check_best_is_selfloop=false)
  {
    std::make_heap(pq.begin(),pq.end());
    finish_adding(check_best_is_selfloop);
    EIFDBG(LAZYF,3,KBESTINFOT("sorted lazy-node="<<this<<": "<<*this));
  }

  bool best_is_selfloop() const
  {
    return top().unary_child() == this;
  }

  // note: if 2nd best is selfloop, you're screwed. violates heap rule temporarily; doing indefinitely would require changing the comparison function. returns true if everything is ok after, false if we still have a selfloop as first-best
  bool postpone_selfloop()
  {
    if (!best_is_selfloop())
      return true;
    if (pq.size() == 1)
      return false;
    if (pq.size()>2 && pq[1] < pq[2]) // STL heap=maxheap. 2 better than 1.
      std::swap(pq[0],pq[2]);
    else
      std::swap(pq[0],pq[1]);
    return !best_is_selfloop();
  }

  /// optional: if you call only add() on sorted, you must finish_adding().
  /// if you added unsorted, don't call this - call sort() instead
  void finish_adding(bool check_best_is_selfloop=false)
  {
    if (pq.empty()) {
      set_first_best(NONE());
      return;
    }
    if (check_best_is_selfloop && !postpone_selfloop())
      throw lazy_derivation_cycle();
    set_first_best(top().derivation);
  }

  // note: postpone_selfloop() may make this not true.
  bool is_sorted()
  {
# ifdef _MSC_VER
    return true; //FIXME: implement
# else
    return __gnu_cxx::is_heap(pq.begin(),pq.end());
# endif
  }

// private:
  typedef std::vector<hyperedge> pq_t;
  typedef std::vector<derivation_type> memo_t;

  //MEMBERS:
  pq_t pq; // INVARIANT: pq[0] contains the last entry added to memo
  memo_t memo;

  //try worsening ith (0=left, 1=right) child and adding to queue
  inline void generate_successor_hyperedge(hyperedge &pending,derivation_type old_parent,lazy_kbest_index_type i)  {
    lazy_forest &child_node=*pending.child[i];
    lazy_kbest_index_type &child_i=pending.childbp[i];
    derivation_type old_child=child_node.memo[child_i];
    EIFDBG(LAZYF,2,
    KBESTINFOT("generate_successor_hyperedge #" << i << " @" << this << ": " << " old_parent="<<old_parent<<" old_child=" <<old_child << " NODE="<<*this);
           KBESTNESTT);

    derivation_type new_child=(child_node.get_best(++child_i));
    if (new_child!=NONE()) { // has child-succesor
      EIFDBG(LAZYF,6,KBESTINFOT("HAVE CHILD SUCCESSOR for i=" << i << ": [" << pending.childbp[0] << ',' << pending.childbp[1] << "]");
             SHOWM7(LAZYF,"generator_successor-child-i",i,pending.childbp[0],pending.childbp[1],old_parent,old_child,new_child,child_node));
      pending.derivation=derivation_factory.make_worse(old_parent,old_child,new_child,i);
      EIFDBG(LAZYF,4,KBESTINFOT("new derivation: "<<pending.derivation));
      push(pending);
    }
    --child_i;
// KBESTINFOT("restored original i=" << i << ": [" << pending.childbp[0] << ',' << pending.childbp[1] << "]");
  }

  void push(const hyperedge &e) {
    pq.push_back(e);
    std::push_heap(pq.begin(),pq.end());
    //This algorithm puts the element at position end()-1 into what must be a pre-existing heap consisting of all elements in the range [begin(), end()-1), with the result that all elements in the range [begin(), end()) will form the new heap. Hence, before applying this algorithm, you should make sure you have a heap in v, and then add the new element to the end of v via the push_back member function.
  }
  void pop() {
    pop_heap(pq.begin(),pq.end());
    //This algorithm exchanges the elements at begin() and end()-1, and then rebuilds the heap over the range [begin(), end()-1). Note that the element at position end()-1, which is no longer part of the heap, will nevertheless still be in the vector v, unless it is explicitly removed.
    pq.pop_back();
  }
  const hyperedge &top() const {
    return pq.front();
  }
};

template <class C, class T, class DF, class FF>
std::basic_ostream<C,T>&
operator<<(std::basic_ostream<C,T>& os, lazy_forest<DF,FF> const& kb)
{
  kb.print(os);
  return os;
}

template <class C, class T, class DF, class FF>
std::basic_ostream<C,T>&
operator<<(std::basic_ostream<C,T>& os, typename lazy_forest<DF,FF>::hyperedge const& h)
{
  h.print(os);
  return os;
}

//FIXME: have to pass these all as params (recursively) to allow concurrent kbests of same type
template <class D,class F>
D lazy_forest<D,F>::derivation_factory;
template <class D,class F>
F lazy_forest<D,F>::filter_factory;
template <class D,class F>
lazy_kbest_stats lazy_forest<D,F>::stats;
template <class D,class F>
bool lazy_forest<D,F>::throw_on_cycle=false;

/// END LIBRARY - only examples/tests follow

#ifdef LAZY_FOREST_EXAMPLES

namespace lazy_forest_kbest_example {

using namespace std;
struct Result {
  struct Factory
  {
    typedef Result *derivation_type;
// static derivation_type NONE,PENDING;
// enum { NONE=0,PENDING=1 };
    static derivation_type NONE() { return (derivation_type)0;}
    static derivation_type PENDING() { return (derivation_type)1;}
    derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child, lazy_kbest_index_type changed_child_index)
    {
      return new Result(prototype,old_child,new_child,changed_child_index);
    }
  };


  Result *child[2];
  string rule;
  string history;
  float cost;
  Result(const string & rule_,float cost_,Result *left=NULL,Result *right=NULL) : rule(rule_),history(rule_,0,1),cost(cost_) {
    child[0]=left;child[1]=right;
    if (child[0]) {
      cost+=child[0]->cost;
      if (child[1])
        cost+=child[1]->cost;
    }
  }
  friend ostream & operator <<(ostream &o,const Result &v)
  {
    o << "cost=" << v.cost << " tree={{{";
    v.print_tree(o,false);
    o <<"}}} derivtree={{{";
    v.print_tree(o);
    return o<<"}}} history={{{" << v.history << "}}}";
  }
  Result(Result *prototype, Result *old_child, Result *new_child,lazy_kbest_index_type which_child) {
    rule=prototype->rule;
    child[0]=prototype->child[0];
    child[1]=prototype->child[1];
    assert(which_child<2);
    assert(child[which_child]==old_child);
    child[which_child]=new_child;
    cost = prototype->cost + - old_child->cost + new_child->cost;
    EIFDBG(LAZYF,7,
           KBESTNESTT;
           KBESTINFOT("NEW RESULT proto=" << *prototype << " old_child=" << *old_child << " new_child=" << *new_child << " childid=" << which_child << " child[0]=" << child[0] << " child[1]=" << child[1]));

    std::ostringstream newhistory,newtree,newderivtree;
    newhistory << prototype->history << ',' << (which_child ? "R" : "L") << '-' << old_child->cost << "+" << new_child->cost;
    //<< '(' << new_child->history << ')';
    history = newhistory.str();
  }
  bool operator < (const Result &other) const {
    return cost > other.cost;
  } // worse < better!
  void print_tree(ostream &o,bool deriv=true) const
  {
    if (deriv)
      o << "["<<rule<<"]";
    else {
      o << rule.substr(rule.find("->")+2,1);
    }
    if (child[0]) {
      o << "(";
      child[0]->print_tree(o,deriv);
      if (child[1]) {
        o << " ";
        child[1]->print_tree(o,deriv);
      }
      o << ")";
    }
  }

};

//Result::Factory::derivation_type Result::Factory::NONE=0,Result::Factory::PENDING=(Result*)0x1;

struct ResultPrinter {
  bool operator()(const Result *r,unsigned i) const {
# if USE_DEBUGPRINT
    KBESTNESTT;
    KBESTINFOT("Visiting result #" << i << " = " << r);
    KBESTINFOT("");
# endif
    cout << "RESULT #" << i << "=" << *r << "\n";
# if USE_DEBUGPRINT
    KBESTINFOT("done #:" << i);
# endif
    return true;
  }
};

typedef lazy_forest<Result::Factory> LK;

/*
  qe
  qe -> A(qe qo) # .33 a
  qe -> A(qo qe) # .33 b
  qe -> B(qo) # .34 c
  qo -> A(qo qo) # .25 d
  qo -> A(qe qe) # .25 e
  qo -> B(qe) # .25 f
  qo -> C # .25 g

  .25 -> 1.37
  .34 -> 1.08
  .33 -> 1.10
*/

inline void jonmay_cycle(unsigned N=25,int weightset=0)
{
  using std::log;
  LK::lazy_forest qe,
    qo;
// float ca=1.1,cb=1.1,cc=1.08,cd=1.37,ce=1.1,cf=1.37,cg=1.37;
  /*
    ca=cb=1.1;
    cc=1.08;
    cd=ce=cf=cg=1.37;
  */
  float ca=.502,cb=.491,cc=0.152,cd=.603,ce=.502,cf=.174,cg=0.01;

  if (weightset==2) {
    ca=cb=cc=cd=ce=cf=cg=1;
  }
  if (weightset==1) {
    ca=cb=-log(.33);
    cc=-log(.34);
    cd=ce=cf=cg=-log(.25);
  }

  Result g("qo->C",cg);
  Result c("qe->B(qo)",cc,&g);
  Result a("qe->A(qe qo)",ca,&c,&g);
  Result b("qe->A(qo qe)",cb,&g,&c);
  Result d("qo->A(qo qo)",cd,&g,&g);
  Result e("qo->A(qe qe)",ce,&c,&c);
  Result f("qo->B(qe)",cf,&c);


  qe.add_sorted(&c,&qo);
  qe.add_sorted(&a,&qe,&qo);
  qe.add_sorted(&b,&qo,&qe);
  assert(qe.is_sorted());

  qo.add(&e,&qe);
  qo.add(&g);
  qo.add(&d,&qo,&qo);
  qo.add(&f,&qe);

  assert(!qo.is_sorted());
  qo.sort();
  assert(qo.is_sorted());
# if USE_DEBUGPRINT
  NESTT;
# endif
  //LK::enumerate_kbest(10,&qo,ResultPrinter());
  qe.enumerate_kbest(N,ResultPrinter());
}

inline void simplest_cycle(unsigned N=25)
{
  float cc=0,cb=1,ca=.33,cn=-10;
  Result c("q->C",cc);
  Result b("q->B(q)",cb,&c);
  Result n("q->N(q)",cn,&c);
  Result a("q->A(q q)",ca,&c,&c);
  LK::lazy_forest q,q2;
  q.add(&a,&q,&q);
  q.add(&b,&q);
  q.add(&n,&q);
  q.add(&c);
  assert(!q.is_sorted());
  q.sort();
// assert(q.is_sorted());
# if USE_DEBUGPRINT
  NESTT;
# endif
  q.enumerate_kbest(N,ResultPrinter());
}

inline void simple_cycle(unsigned N=25)
{

  float cc=0;
  Result c("q->C",cc);
  float cd=.01;
  Result d("q2->D(q)",cd,&c);
  float cneg=-10;
  Result eneg("q2->E(q2)",cneg,&d);
  float cb=.1;
  Result b("q->B(q2 q2)",cb,&d,&d);
  float ca=.5;
  Result a("q->A(q q)",ca,&c,&c);
  LK::lazy_forest q,q2;
  q.add(&a,&q,&q);
  q.add(&b,&q2,&q2);
  q.add(&c);
  assert(!q.is_sorted());
  q.sort();
  assert(q.is_sorted());
  q2.add_sorted(&d,&q);
  q2.add_sorted(&eneg,&q2);
// assert(q2.is_sorted());
// q2.sort();
// DBP2(*q2.pq[0].derivation,*q2.pq[1].derivation);
  // assert(!q2.is_sorted());
//  DBP2(eneg,d);
//  NESTT;
  q.enumerate_kbest(N,ResultPrinter());
}


inline void jongraehl_example(unsigned N=25)
{
  float eps=-1000;
  LK::lazy_forest a,b,c,f;
  float cf=-2;Result rf("f->F",cf);
  float cb=-5;Result rb("b->B",cb);
  float cfb=eps;Result rfb("f->I(b)",cfb,&rb);
  float cbf=-cf+eps;Result rbf("b->H(f)",cbf,&rf);
  float cbb=10;Result rbb("b->G(b)",cbb,&rb);
  float ccbf=8;Result rcbf("c->C(b f)",ccbf,&rb,&rf);
  float cabc=6;Result rabc("a->A(b c)",cabc,&rb,&rcbf);
  float caa=-cabc+eps;Result raa("a->Z(a)",caa,&rabc);
// Result ra("a",6),rb("b",5),rc("c",1),rf("d",2),rb2("B",5),rb3("D",10),ra2("A",12);
  f.add_sorted(&rf); // terminal
  f.add_sorted(&rfb);
  b.add_sorted(&rb); // terminal
  b.add_sorted(&rbf,&f);
  b.add_sorted(&rbb,&b);
  c.add_sorted(&rcbf,&b,&f);
  a.add_sorted(&rabc,&b,&c);
  a.add_sorted(&raa,&a);
  assert(a.is_sorted());
  assert(b.is_sorted());
  assert(c.is_sorted());
  assert(f.is_sorted());
  NESTT;
  LK::throw_on_cycle=true;
  f.enumerate_kbest(1,ResultPrinter());
  b.enumerate_kbest(1,ResultPrinter());
  c.enumerate_kbest(1,ResultPrinter());
  a.enumerate_kbest(N,ResultPrinter());
}

inline void all_examples(unsigned N=30)
{
  simplest_cycle(N);
  simple_cycle(N);
  jongraehl_example(N);
  jonmay_cycle(N);
}

}//lazy_forest_kbest_example
#endif

# ifdef TEST
BOOST_AUTO_TEST_CASE(TEST_lazy_kbest) {
  lazy_forest_kbest_example::all_examples();

}
#endif

}//graehl

#ifdef LAZY_FOREST_SAMPLE
int main()
{
  using namespace graehl::lazy_forest_kbest_example;
  unsigned N=30;
  simplest_cycle(N);
// all_examples();
  return 0;
  /*
    if (argc>1) {
    char c=argv[1][0];
    if (c=='1')
    simple_cycle();
    else if (c=='2')
    simplest_cycle();
    else
    jongraehl_example();
    } else
    jonmay_cycle();
  */
}
#endif

#endif
