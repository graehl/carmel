#ifndef WEIGHT_H
#define WEIGHT_H

/*
All these are legal input (and output) from Carmel, SuperCarmel?, and ForestEM?:

0
e^-2.68116e+11
-2.68116e+11ln
0.0952381
e^-4086.42
-4086.42ln
0.047619
1

UPDATE: so that nobody has to be confused about what the "ln" suffix means, the new default output shall be "e^n" (except in case of zero, which is just "0" and not "e^-INF", since "-INF" doesn't read/write properly using the standard I/O libraries). I suggest everyone only produce such outputs, but be prepared to read arbitrary real numbers, as well as those starting with "e^".

That is: either a number in the form supported by C/C++ i/o of double floating point numbers, OR such a number (call it N) written "e^N", OR (deprecated) written "Nln", indicating that the number is actually e^N. e.g. e^0=1.

\forall N,Nln=eN

There is well-tested log-rep addition/subtraction code in GraehlCVS (graehl/shared/weight.h).

Carmel optionally supports the use of base 10 instead: \forall N,Nlog=10^N, but that is no longer tolerated - e is the only natural base (and "log" sometimes means base 2, like in information theory, so it's confusing).
*/

#include "config.h"
#include "myassert.h"
#include "genio.h"
#include "funcs.hpp"
#include "threadlocal.hpp"



#ifdef _MSC_VER
#pragma warning(push)
#endif
#include <cmath>
#include <iostream>

#ifdef _MSC_VER
// Microsoft C++: conversion from double to FLOAT_TYPE:
#pragma warning(disable:4244)
#endif

//! warning: unless #ifdef WEIGHT_CORRECT_ZERO
// Weight(0) will may give bad results when computed with, depending on math library behavior
// defining WEIGHT_CORRECT_ZERO will incur a performance penalty

template <class Real>
struct Weight {                 // capable of representing nonnegative reals
  // internal implementation note: by their base e logarithm
  private:
    const Real HUGE_FLOAT = (Real)(HUGE_VAL*HUGE_VAL);
    enum { DEFAULT_BASE=0,LN=1,LOG10=2,EXP=3 }; // EXP is same as LN but write e^10e-6 not 10e-6ln
    enum { DEFAULT_LOG=0,ALWAYS_LOG=1, SOMETIMES_LOG=2, NEVER_LOG=3 };
  // IEE float safe till about 10^38, loses precision earlier (10^32?) or 2^127 -> 2^120
  // 32 * ln 10 =~ 73
  // double goes up to 2^1027, loses precision at say 2^119?  119 * ln 2 = 82
  enum {LN_TILL_UNDERFLOW=(sizeof(Real)==4? 73 : 82)} ;

  static const int base_index; // handle to ostream iword for LogBase enum (initialized to 0)
  static const int thresh_index; // handle for OutThresh
    static THREADLOCAL int default_base;
    static THREADLOCAL int default_thresh;
  public:
  //  Weight() : weight(-HUGE_FLOAT) {}
  Weight() { setZero(); }
  Weight(bool,bool) { setInfinity(); }
  Weight(double f) {
    setReal(f);
  }
  static const Weight<Real> ZERO, INF;
  // linux g++ 3.2 didn't like static self-class member
  static const Real FLOAT_INF() {
        return HUGE_FLOAT;
  }
  Real weight;

  // output format manipulators: cout << Weight<Real>::out_log10;
    static void default_log10() {
        default_base=LOG10;
    }
    static void default_ln() {
        default_base=LN;
    }
    static void default_exp() {
        default_base=EXP;
    }
    static void default_sometimes_log() {
        default_thresh=SOMETIMES_LOG;
    }
    static void default_always_log() {
        default_thresh=ALWAYS_LOG;
    }
    static void default_never_log() {
        default_thresh=NEVER_LOG;
    }

    template <class charT, class Traits>
    static int get_log(std::basic_ostream<charT,Traits>& o) {
        int thresh=o.iword(thresh_index);
        if (thresh == DEFAULT_LOG)
            thresh=default_thresh;
        return thresh;
    }

    template <class charT, class Traits>
    static int get_log_base(std::basic_ostream<charT,Traits>& o) {
        int thresh=o.iword(base_index);
        if (thresh == DEFAULT_BASE)
            thresh=default_base;
        return thresh;
    }

    template<class A,class B> static std::basic_ostream<A,B>&
    out_default_base(std::basic_ostream<A,B>& os);
  template<class A,class B> static std::basic_ostream<A,B>&
    out_log10(std::basic_ostream<A,B>& os);
  template<class A,class B> static std::basic_ostream<A,B>&
    out_exp(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_ln(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_sometimes_log(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_always_log(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_never_log(std::basic_ostream<A,B>& os);

    template<class A,class B> static std::basic_ostream<A,B>&
    out_default_log(std::basic_ostream<A,B>& os);


  static Weight<Real> result;
  // default = operator:

  //double toFloat() const {
  //return getReal();
  //}

  double getReal() const {
    return std::exp(weight);
  }
  void setRandomFraction() {
        setReal(random_pos_fraction());
  }
  Real getLog(Real base) const {
    return weight / log(base);
  }
  Real getLogImp() const {
    return weight;
  }
  typedef Real cost_type;
  cost_type getCost() const {
        return -weight;
  }
  void setCost(Real f) {
        weight=-f;
  }

  Real getLn() const {
    return weight;
  }
  Real getLog10() const {
    static const Real oo_ln10 = 1./log(10.f);
    return oo_ln10 * weight;
  }
  bool fitsInReal() const {
    return isZero() || (getLn() < LN_TILL_UNDERFLOW && getLn() > -LN_TILL_UNDERFLOW);
  }
  bool isInfinity() const {
    return weight == HUGE_FLOAT;
  }
  void setInfinity() {
    weight = HUGE_FLOAT;
  }

  bool isZero() const {
//    return weight == -HUGE_FLOAT;
        return !isPositive();
  }
  bool isPositive() const {
    return weight > -HUGE_FLOAT;
  }
  void setZero() {
    weight = -HUGE_FLOAT;
  }
  bool isOne() const {
        return weight==0;
  }
  void setOne() {
        weight=0;
  }
  void setReal(double f) {
    if (f > 0)
      weight=(Real)log(f);
    else
      setZero();
  }
        void NaNCheck() const {
#ifdef DEBUG
          if(weight!=weight) {
                *(int*)0=0;
                Assert(weight==weight);
          }
#else
                assert(weight==weight);
#endif
        }
  void setLn(Real w) {
    weight=w;
  }
  void setLog10(Real w) {
    static const Real ln10 = log(10.f);
    weight=w*ln10;
  }

  friend Weight<Real> operator + (Weight<Real>, Weight<Real>);
  friend Weight<Real> operator - (Weight<Real>, Weight<Real>);
  friend Weight<Real> operator * (Weight<Real>, Weight<Real>);
  friend Weight<Real> operator / (Weight<Real>, Weight<Real>);
  Weight<Real> operator += (Weight<Real> w)
  {
    *this = *this + w;
    return *this;
  }
  Weight<Real> operator -= (Weight<Real> w)
  {
    *this = *this - w;
    return *this;
  }
  Weight<Real> operator *= (Weight<Real> w)
  {
#ifdef WEIGHT_CORRECT_ZERO
    if (!isZero())
#endif
                        weight += w.weight;
    return *this;
  }
  Weight<Real> operator /= (Weight<Real> w)
  {
                                Assert(!w.isZero());
#ifdef WEIGHT_CORRECT_ZERO
//              if (w.isZero())
                //      weight = HUGE_FLOAT;
    //else
                        if (!isZero())
#endif
            weight -= w.weight;

    return *this;
  }
  Weight<Real> raisePower(Real power) {
#ifdef WEIGHT_CORRECT_ZERO
                if (!isZero())
#endif
                        weight *= power;
    return *this;
  }
  Weight<Real> invert() {
    weight = -weight;
    return *this;
  }
  Weight<Real> takeRoot(Real nth) {
#ifdef WEIGHT_CORRECT_ZERO
                if (!isZero())
#endif
                        weight /= nth;
    return *this;
  }
  Weight<Real> operator ^= (Real power) { // raise Weight<Real>^power
    raisePower(power);
    return *this;
  }

  template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& os) const;
template <class charT, class Traits>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& os);

};

template <class Real>
inline Real log(Weight<Real> a) {
    return a.getLn();
}

template<class T> inline T exponential(Real exponent);

template <class Real>
inline
Weight<Real> exponential<Weight<Real> >(Real exponent) {
    Weight<Real> r;
    r.setLn(exponent);
    return r;
}

/*
#include "semiring.hpp"
template <>
struct semiring_traits<Weight<Real>> {
    typedef Weight<Real> value_type;
    static inline value_type exponential(Real exponent) {
        return exponential<C>(exponent);
    }
    static inline void set_one(Weight<Real> &w) { w.setOne(); }
    static inline void set_zero(Weight<Real> &w) { w.setZero(); }
    static inline bool is_one(const Weight<Real> &w) { w.isOne(); }
    static inline bool is_zero(const Weight<Real> &w) { w.isZero(); }
    static inline void addto(Weight<Real> &w,Weight<Real> p) {
        w += p;
    }

};
*/

template <class Real>
inline Weight<Real> pow_logexponent(Weight<Real> a, Weight<Real> b) {
        a.raisePower(b.weight);
        return a;
}

template <class Real>
inline Weight<Real> root(Weight<Real> w,Real nth) {
        w.takeRoot(nth);
        return w;
}

template <class Real>
inline Weight<Real> pow(Weight<Real> w,Real nth) {
        w.raisePower(nth);
        return w;
}

template <class Real>
inline Weight<Real> operator ^(Weight<Real> base,Real exponent) {
        return pow(base,exponent);
}



template <class charT, class Traits, class Real>
std::ios_base::iostate Weight<Real>::print_on(std::basic_ostream<charT,Traits>& o) const
{
    int base=Weight<Real>::get_log_base(o);
        if ( isZero() )
                o << "0";
        else {
            int log=Weight<Real>::get_log(o);

            if ( (log == Weight<Real>::SOMETIMES_LOG && fitsInReal()) || log == Weight<Real>::NEVER_LOG ) {
                o << getReal();
            } else { // out of range or ALWAYS_LOG
                if ( base == Weight<Real>::LN) {
                        o << getLn() << "ln";
                } else if (base == Weight<Real>::LOG10) {
                        o << getLog10() << "log";
                } else {
                    o << "e^" << getLn();
                }
            }
        }
        return GENIOGOOD;
}

template <class charT, class Traits, class Real>
std::ios_base::iostate Weight<Real>::get_from(std::basic_istream<charT,Traits>& in)
{
  char c;
  double f=0;
  EXPECTI_FIRST(in >> c);
  if (c != 'e')
      in.unget();
  else {
      EXPECTCH('^');
      EXPECTI(in >> f);
      setLn((Real)f);
      return GENIOGOOD;
  }
  in >> f;

  if ( in.fail() )
      goto fail;
  if ( in.eof() ) {
    setReal(f);
  } else if ( (c = in.get()) == 'l' ) {
   char n = in.get();
   if ( n == 'n')
    setLn((Real)f);
   else if ( n == 'o' && in.get() == 'g' )
    setLog10(f);
   else {
    setZero();
        return GENIOBAD;
   }
  } else {
    in.unget();
    setReal(f);
  }
  return GENIOGOOD;
  fail:
  setZero();
  return GENIOBAD;
}



#include "genio.h"

template <class charT, class Traits, class Real>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, Weight<Real> &arg)
{
        return gen_extractor(is,arg);
}

template <class charT, class Traits, class Real>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const Weight<Real> &arg)
{
        return gen_inserter(os,arg);
}


//std::ostream& operator << (std::ostream &o, Weight<Real> weight);

//std::istream& operator >> (std::istream &i, Weight<Real> &weight);

/*
bool operator == (Weight<Real> lhs, Weight<Real> rhs);
bool operator != (Weight<Real> lhs, Weight<Real> rhs);

//using namespace std;


Weight<Real> operator *(Weight<Real> lhs, Weight<Real> rhs);

Weight<Real> operator /(Weight<Real> lhs, Weight<Real> rhs);

Weight<Real> operator +(Weight<Real> lhs, Weight<Real> rhs);

Weight<Real> operator -(Weight<Real> lhs, Weight<Real> rhs);

bool operator <(Weight<Real> lhs, Weight<Real> rhs);
bool operator >(Weight<Real> lhs, Weight<Real> rhs);
bool operator <=(Weight<Real> lhs, Weight<Real> rhs);
bool operator >=(Weight<Real> lhs, Weight<Real> rhs);
*/

template <class Real>
inline bool operator == (Weight<Real> lhs, Weight<Real> rhs) { return lhs.weight == rhs.weight; }
template <class Real>
inline bool operator != (Weight<Real> lhs, Weight<Real> rhs) { return lhs.weight != rhs.weight; }

inline Weight<Real> operator *(Weight<Real> lhs, Weight<Real> rhs) {
  Weight<Real> result;
  result.weight =  lhs.weight + rhs.weight;
  return result;
}

template <class Real>
inline Weight<Real> operator /(Weight<Real> lhs, Weight<Real> rhs) {
  Weight<Real> result;
  result.weight =  lhs.weight - rhs.weight;
  return result;
}

#define MUCH_BIGGER_LN (sizeof(Real)==4? 16.f : 36.)
    
// represents BIG=10^MUCH_BIGGER_LN - if X > BIG*Y, then X+Y =~ X
// 32 bit Real IEEE Real has 23 binary digit mantissa, so
// can represent about 16 base E digits only
// fixme: if you use 64-bit doubles instead of floats, 52 binary digits
// so define as 36.f instead


template <class Real>
inline Weight<Real> operator +(Weight<Real> lhs, Weight<Real> rhs) {
  //fixme: below test is needed with glibc without -ffast-math to compute 0+0 properly (?)
  //  if (lhs == 0.0)
  //  return rhs;
#ifdef WEIGHT_CORRECT_ZERO
        if (lhs.isZero())
                return rhs;
        if (rhs.isZero())
                return lhs;
#endif

  Real diff = lhs.weight - rhs.weight;
  if ( diff > MUCH_BIGGER_LN )
    return lhs;
  if ( diff < -MUCH_BIGGER_LN )
    return rhs;

  Weight<Real> result;

  if ( diff < 0 ) { // rhs is bigger
          result.weight = (Real)(rhs.weight + log(1 + std::exp(lhs.weight-rhs.weight)));
    return result;
  }
  // lhs is bigger
  result.weight = (Real)( lhs.weight + log(1 + std::exp(rhs.weight-lhs.weight)));
  return result;
}

template <class Real>
inline Weight<Real> operator -(Weight<Real> lhs, Weight<Real> rhs) {

#ifdef WEIGHT_CORRECT_ZERO
        if (rhs.isZero())
                return lhs;
#endif

  Weight<Real> result;
        Real rdiff=rhs.weight-lhs.weight;
  if ( rdiff >= 0 )        // lhs <= rhs
          // clamp to zero as minimum without giving exception (not mathematically correct!)
  {
    //result.weight = -HUGE_FLOAT; // default constructed to this already
    return result;
  }


  if ( rdiff < -MUCH_BIGGER_LN ) // lhs >> rhs
          return lhs;

  // lhs > rhs

  result.weight = (Real)(lhs.weight + log(1 - std::exp(rdiff)));
  return result;
}

template <class Real>
inline Weight<Real> absdiff(Weight<Real> lhs, Weight<Real> rhs) {
#if 0
        // UNTESTED
        Real diff=lhs.weight-rhs.weight;
        if ( diff > MUCH_BIGGER_LN )
                return lhs;
        if ( diff < -MUCH_BIGGER_LN )
                return rhs;
        Weight<Real> result;
        if ( diff < 0 )
                result.weight = (Real)(rhs.weight + log(1 - std::exp(diff)));
        else
                result.weight = (Real)(lhs.weight + log(1 - std::exp(-diff)));
#else
        if (lhs.weight > rhs.weight)
                return lhs-rhs;
        else
                return rhs-lhs;
#endif
}

template <class Real>
inline bool operator <(Weight<Real> lhs, Weight<Real> rhs) { return lhs.weight < rhs.weight; }
template <class Real>
inline bool operator >(Weight<Real> lhs, Weight<Real> rhs) { return lhs.weight > rhs.weight; }
template <class Real>
inline bool operator <=(Weight<Real> lhs, Weight<Real> rhs) { return lhs.weight <= rhs.weight; }
template <class Real>
inline bool operator >=(Weight<Real> lhs, Weight<Real> rhs) { return lhs.weight >= rhs.weight; }

#ifdef _MSC_VER
#pragma warning(pop)

//template<class A,class B> std::basic_ostream<A,B>& operator <<(std::basic_ostream<A,B>& os, std::basic_ostream<A,B>& (*manip)(std::basic_ostream<A,B>&)) { return manip(os);}
// in MSVC++ release optimization mode, manipulators don't work by os << manip, must do manip(os) =(
#endif

template<class A,class B>
//__declspec(noinline)
std::basic_ostream<A,B>&
Weight<Real>::out_log10(std::basic_ostream<A,B>& os)

 { os.iword(base_index) = LOG10; return os; }

template<class A,class B,class Real> std::basic_ostream<A,B>&
Weight<Real>::out_default_base(std::basic_ostream<A,B>& os) { os.iword(base_index) = DEFAULT_BASE; return os; }

template<class A,class B,class Real> std::basic_ostream<A,B>&
Weight<Real>::out_ln(std::basic_ostream<A,B>& os) { os.iword(base_index) = LN; return os; }

template<class A,class B> std::basic_ostream<A,B>&
Weight<Real>::out_exp(std::basic_ostream<A,B>& os) { os.iword(base_index) = EXP; return os; }

template<class A,class B,class Real> std::basic_ostream<A,B>&
Weight<Real>::out_sometimes_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = SOMETIMES_LOG; return os; }

template<class A,class B,class Real> std::basic_ostream<A,B>&
Weight<Real>::out_always_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = ALWAYS_LOG; return os; }

template<class A,class B,class Real> std::basic_ostream<A,B>&
Weight<Real>::out_never_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = NEVER_LOG; return os; }

template<class A,class B,class Real> std::basic_ostream<A,B>&
Weight<Real>::out_default_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = DEFAULT_LOG; return os; }


template <class Real>
void inline dbgout(std::ostream &o,Weight<Real> w) {
    Weight<Real>::out_never_log(o);
    o << w;
#ifdef VERBOSE_DEBUG
      o << '=';
      Weight<Real>::out_always_log(o)
        o << w;
#endif
}

#include <limits>
namespace std {
template <class Real>
class numeric_limits<Weight<Real> > {
public:
  static bool has_infinity() { return true; }
    enum { is_specialized=1,digits10=std::numeric_limits<Real>::digits10 };

  //FIXME: add rest
};
};

#ifdef DEBUGNAN
#define NANCHECK(w) w.NaNCheck()
#else
#define NANCHECK(w)
#endif

#ifdef MAIN
#include "weight.cc"
#endif

#endif
