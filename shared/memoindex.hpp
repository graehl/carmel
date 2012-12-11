// generic memoization
#ifndef MEMOINDEX_HPP
#define MEMOINDEX_HPP

#include <graehl/shared/2hash.h>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

//F: single-argument (adaptable) functor with argument_type result_type
// scratch that: void F(const arg &,ret &) (don't trust in temp return opt)
// also, f.memo=register(this) ... for (co)recursive memo calls
// something that isn't checked for but could be very bad ... use of result from self before computed - but if f is a true function, any call f(a) from f(a) SHOULD result in infinite loop.  might detect this in debug mode?  but for f not true function, may want to allow ... then calling f(a) needs to memo->find(a) to detect, or make sure result_type is default-initialized to a known value.
// note that it would be just as good to leave storing of results up to f itself, i.e. we only give the index as Result instead of pair<ret,index> (f would have to keep a dynamic array and check if it needs to push_back, sorta like Alphabet)
template <class F>
struct MemoIndex {
  unsigned &n; // # of entries, also next index for new Arg
  F &f;
  typedef typename F::argument_type Arg;
  typedef typename F::result_type Ret;

  typedef std::pair<Ret,unsigned> Result;
  typedef HashTable<Arg,Result>  Table;
  Table memo;
  explicit MemoIndex(unsigned &n_,F &f_): n(n_),f(f_) {f.set_memo(this);}

  Result *find(const Arg &a) {
        return find_second(memo,a);
  }
  Result & apply(const Arg &a) {
        typename Table::insert_result_type i=memo.insert(a); // non-STL: =insert(pair<Arg,Result>(a,Result()))
        Result &r=i.first->second;
        if (i.second) { // new
          r.second=n++; // very important to do this before invoking functor (in case it recursively uses this Memo!).  note that result gets default constructed; if you care to distinguish between really-finished and in-progress, make sure default constructed is unique.
          f(a,r.first);
          // something to mark f(a) finished so we detect (semantically incorrect?) loops?
        }
        return r;
        /*
        Result *f=find_second(memo,a);
        if (f)
          return *f;
        Result &r=memo[a];
        */
  }
};

// when you aren't going to share index-space with anyone else:
template <class F>
struct MemoIndexOwn : public MemoIndex<F> {
  unsigned next;
  explicit MemoIndexOwn(F &f_): MemoIndex<F>(next,f_),next(0) {}
};

// inherit from this and overload
template <class Arg,class Ret>
struct MemoFn {
  typedef Arg argument_type;
  typedef Ret result_type;
  void operator()(const argument_type &a,result_type &ret) {
  }
  void set_memo(MemoIndex<MemoFn> *memo) {
  }
};


#ifdef GRAEHL_TEST

struct ExampleF : public MemoFn<int,int> {
  MemoIndex<ExampleF> *memo;
  int n;
  ExampleF() : n(0) { }
  void operator()(const argument_type &a,result_type &ret) {
        ++n;
//      DBP("\nn="<<n<<" arg="<<a<<std::endl);
        if (a%3)
          ret=memo->apply(a-1).first;
        else
      ret=(a%2);

  }
  void set_memo(MemoIndex<ExampleF> *memo_) {
        memo=memo_;
  }
};
BOOST_AUTO_TEST_CASE( memoindex )
{
  {
  unsigned start=0;
  ExampleF f;
  MemoIndex<ExampleF> m(start,f);
  BOOST_CHECK(m.apply(1).first==0);
  BOOST_CHECK(m.apply(1).second==0);
  BOOST_CHECK(m.find(1)!=0);
  BOOST_CHECK(m.find(0)!=0);
  BOOST_CHECK(m.find(1)==&m.apply(1));

  BOOST_CHECK(m.find(0)==&m.apply(0));
  BOOST_CHECK(m.find(1)==&m.apply(1));
  BOOST_CHECK(m.apply(0).second==1);
  MemoIndex<ExampleF>::Result *p3=&m.apply(3);
  BOOST_CHECK(m.find(3)==p3);
  BOOST_CHECK(m.apply(3).first==1);
  BOOST_CHECK(!m.find(2));
  BOOST_CHECK(start == 3);
  BOOST_CHECK(m.f.n == 3);
  }
  {
  ExampleF f;
  MemoIndexOwn<ExampleF> m(f);
  BOOST_CHECK(m.apply(1).first==0);
  BOOST_CHECK(m.apply(1).second==0);
  BOOST_CHECK(m.find(1)!=0);
  BOOST_CHECK(m.find(0)!=0);
  BOOST_CHECK(m.find(1)==&m.apply(1));

  BOOST_CHECK(m.find(0)==&m.apply(0));
  BOOST_CHECK(m.find(1)==&m.apply(1));
  BOOST_CHECK(m.apply(0).second==1);
  MemoIndexOwn<ExampleF>::Result *p3=&m.apply(3);
  BOOST_CHECK(m.find(3)==p3);
  BOOST_CHECK(m.apply(3).first==1);
  BOOST_CHECK(!m.find(2));
  BOOST_CHECK(m.f.n == 3);
  }
}
#endif
}
#endif
