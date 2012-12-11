#ifndef GRAEHL_SHARED__NORMALIZE_RANGE_HPP
#define GRAEHL_SHARED__NORMALIZE_RANGE_HPP

#include <graehl/shared/range.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/value_type.hpp>

namespace graehl {

template <class R>
typename range_value<R>::type sum(R const& r,typename range_value<R>::type z=typename range_value<R>::type()) {
  return boost::accumulate(r,z);
}

template <class W>
struct normalize_options {
  W addk_num;
  W addk_denom;
  explicit normalize_options(W addk_num=W(),W addk_denom=W()) : addk_num(addk_num),addk_denom(addk_denom) {}
  void addUnseen(W n) {
    addk_denom+=n*addk_num; // there are n tokens with 0 count; intend addk_num to apply to them also
  }
  static char const* caption() { return "Probability Normalization"; }
  template <class OD>
  void add_options(OD &optionsDesc) {
    optionsDesc.add_options()
      .defaulted("addk,k",&addk_num,"add counts of [addk] to every observed event before normalizing")
      .defaulted("denom-addk,K",&addk_denom,"add [denom-addk] to the denominator only when normalizing")
      ;
  }
  template <class Config>
  void configure(Config &c)
  {
    c.is("add-k normalization");
    c('k')("addk,k",&addk_num)("add counts of [addk] to every observed event before normalizing");
    c('K')("denom-addk,K",&addk_denom)("add [denom-addk] to the denominator only when normalizing");
  }

  void validate() {}
};

template <class W>
struct normalize_addk {
  W knum;
  W denom;
  normalize_addk(std::size_t N,W sum_unsmoothed,W addk_num=0,W addk_denom=0) : knum(addk_num) {
    denom=sum_unsmoothed+knum*N+addk_denom;
  }
  normalize_addk(std::size_t N,W sum_unsmoothed,normalize_options<W> const& n) : knum(n.addk_num) {
    denom=sum_unsmoothed+knum*N+n.addk_denom;
  }
  W operator()(W c) const { return (c+knum)/denom; } // maybe some semirings don't have * (1/denom) but have / denom ?
};

template <class R,class O>
O normalize_copy(typename range_value<R>::type sum,R const&r,O o,normalize_options<typename range_value<R>::type > n=normalize_options<typename range_value<R>::type >()) {
  typedef typename range_value<R>::type V;
  return boost::transform(r,o,normalize_addk<V>(size(r),sum,n));
}

template <class R,class O>
O normalize_copy(R const&r,O o,normalize_options<typename range_value<R>::type > n=normalize_options<typename range_value<R>::type >()) {
  return normalize_copy(sum(r),r,boost::begin(r),n);
}

template <class R>
void normalize_sum(typename range_value<R>::type sum,R &r,normalize_options<typename range_value<R>::type > n=normalize_options<typename range_value<R>::type >()) {
  normalize_copy(sum,r,boost::begin(r),n);
}

template <class R>
void normalize(R &r,normalize_options<typename range_value<R>::type > n=normalize_options<typename range_value<R>::type >()) {
  normalize_sum(sum(r),r,n);
}

}//ns


#endif
