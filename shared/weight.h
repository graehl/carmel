#ifndef WEIGHT_H 
#define WEIGHT_H 1
#include "config.h"
#include "myassert.h"
#include "genio.h"

#ifdef _MSC_VER
#pragma warning(push)
#endif
#include <cmath>
#include <iostream>

#ifdef _MSC_VER
// Microsoft C++: conversion from double to FLOAT_TYPE:
#pragma warning(disable:4244)
#endif

static FLOAT_TYPE HUGE_FLOAT = (FLOAT_TYPE)(HUGE_VAL*HUGE_VAL);

//! warning: unless #ifdef WEIGHT_CORRECT_ZERO
// Weight(0) will may give bad results when computed with, depending on math library behavior
// defining WEIGHT_CORRECT_ZERO will incur a performance penalty

#include "functors.hpp"

struct Weight {			// capable of representing nonnegative reals 
  // internal implementation note: by their base e logarithm
  private:
  enum { LN=0,LOG10, };
  enum { ALWAYS_LOG=1, VAR, ALWAYS_REAL };
  // IEE float safe till about 10^38, loses precision earlier (10^32?) or 2^127 -> 2^120
  // 32 * ln 10 =~ 73
  // double goes up to 2^1027, loses precision at say 2^119?  119 * ln 2 = 82
  enum {LN_TILL_UNDERFLOW=(sizeof(FLOAT_TYPE)==4? 73 : 82)} ;

  static const int base_index; // handle to ostream iword for LogBase enum (initialized to 0)
  static const int thresh_index; // handle for OutThresh

  public:	
  static const Weight ZERO, INF;
  // linux g++ 3.2 didn't like static self-class member
  static const FLOAT_TYPE FLOAT_INF() {
	return HUGE_FLOAT;
  }
  FLOAT_TYPE weight;

  // output format manipulators: cout << Weight::out_log10;

  template<class A,class B> static std::basic_ostream<A,B>&
    out_log10(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_ln(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_variable(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_always_log(std::basic_ostream<A,B>& os);

  template<class A,class B> static std::basic_ostream<A,B>&
    out_always_real(std::basic_ostream<A,B>& os);

  static Weight result;
  // default = operator:

  //double toFloat() const { 
  //return getReal();
  //}

  double getReal() const {
    return std::exp(weight);
  }
  void setRandomFraction() {
	setReal(rand_pos_fraction());
  }
  FLOAT_TYPE getLog(FLOAT_TYPE base) const {
    return weight / log(base);
  }
  FLOAT_TYPE getLogImp() const {
    return weight;
  }
  typedef FLOAT_TYPE cost_type;
  cost_type getCost() const {
	return -weight;
  }
  void setCost(FLOAT_TYPE f) {
	weight=-f;
  }

  FLOAT_TYPE getLn() const {
    return weight;
  }
  FLOAT_TYPE getLog10() const {
    static const FLOAT_TYPE oo_ln10 = 1./log(10.f);
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
      weight=(FLOAT_TYPE)log(f);
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
  void setLn(FLOAT_TYPE w) {
    weight=w;
  }
  void setLog10(FLOAT_TYPE w) {
    static const FLOAT_TYPE ln10 = log(10.f);
    weight=w*ln10;
  }

  //  Weight() : weight(-HUGE_FLOAT) {}
  Weight() { setZero(); }
  Weight(bool,bool) { setInfinity(); }
  Weight(double f) {
    setReal(f);
  }
  friend Weight operator + (Weight, Weight);
  friend Weight operator - (Weight, Weight);
  friend Weight operator * (Weight, Weight);
  friend Weight operator / (Weight, Weight);
  Weight operator += (Weight w)
  {
    *this = *this + w;
    return *this;
  }
  Weight operator -= (Weight w)
  {
    *this = *this - w;
    return *this;
  }
  Weight operator *= (Weight w)
  {
#ifdef WEIGHT_CORRECT_ZERO
    if (!isZero())
#endif
			weight += w.weight;
    return *this;
  }
  Weight operator /= (Weight w)
  {
				Assert(!w.isZero());
#ifdef WEIGHT_CORRECT_ZERO
//		if (w.isZero())
		//	weight = HUGE_FLOAT;
    //else 
			if (!isZero())
#endif
	    weight -= w.weight;

    return *this;
  }
  Weight raisePower(FLOAT_TYPE power) {
#ifdef WEIGHT_CORRECT_ZERO
		if (!isZero())
#endif
			weight *= power;
    return *this;
  }
  Weight invert() {
    weight = -weight;
    return *this;
  }
  Weight takeRoot(FLOAT_TYPE nth) {
#ifdef WEIGHT_CORRECT_ZERO
		if (!isZero())
#endif
			weight /= nth;
    return *this;
  }
  Weight operator ^= (FLOAT_TYPE power) { // raise Weight^power
    raisePower(power);
    return *this;
  }

  template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& os) const;
template <class charT, class Traits>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& os);

};

inline FLOAT_TYPE log(Weight a) {
    return a.getLn();
}

template<class T> inline T exponential(FLOAT_TYPE exponent);

template <>
inline
Weight exponential<Weight>(FLOAT_TYPE exponent) {
    Weight r;
    r.setLn(exponent);
    return r;
}

/*
#include "semiring.hpp"
template <>
struct semiring_traits<Weight> {
    typedef Weight value_type;
    static inline value_type exponential(FLOAT_TYPE exponent) {            
        return exponential<C>(exponent);
    }
    static inline void set_one(Weight &w) { w.setOne(); }
    static inline void set_zero(Weight &w) { w.setZero(); }
    static inline bool is_one(const Weight &w) { w.isOne(); }
    static inline bool is_zero(const Weight &w) { w.isZero(); }
    static inline void addto(Weight &w,Weight p) {
        w += p;
    }
    
};
*/

inline Weight pow_logexponent(Weight a, Weight b) {
	a.raisePower(b.weight);
	return a;
}

inline Weight root(Weight w,FLOAT_TYPE nth) {
	w.takeRoot(nth);
	return w;
}

inline Weight pow(Weight w,FLOAT_TYPE nth) {
	w.raisePower(nth);
	return w;
}


inline Weight operator ^(Weight base,FLOAT_TYPE exponent) {
	return pow(base,exponent);
}



template <class charT, class Traits>
std::ios_base::iostate Weight::print_on(std::basic_ostream<charT,Traits>& o) const
{
	if ( isZero() )
		o << "0";
	else if ( (o.iword(thresh_index) == VAR && fitsInReal()) || o.iword(thresh_index) == ALWAYS_REAL ) {
		o << getReal();
	} else { // out of range or ALWAYS_LOG
		if (o.iword(base_index) == LN) {
			o << getLn() << "ln";
		} else { // LOG10
			o << getLog10() << "log";
		}
	}
	return GENIOGOOD;
}

template <class charT, class Traits>
std::ios_base::iostate Weight::get_from(std::basic_istream<charT,Traits>& i)
{
  char c;
  double f=0;
  i >> f;
  
  if ( i.fail() ) {
	setZero();
	return GENIOBAD;
  } else if ( i.eof() ) {
    setReal(f);
  } else if ( (c = i.get()) == 'l' ) {
   char n = i.get();  	
   if ( n == 'n')
    setLn((FLOAT_TYPE)f);
   else if ( n == 'o' && i.get() == 'g' )
    setLog10(f);
   else {
    setZero();
	return GENIOBAD;
   }
  } else {
    i.unget();
    setReal(f);
  }
  return GENIOGOOD;
}



#include "genio.h"

template <class charT, class Traits>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, Weight &arg)
{
	return gen_extractor(is,arg);
}

template <class charT, class Traits>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const Weight &arg)
{
	return gen_inserter(os,arg);
}


//std::ostream& operator << (std::ostream &o, Weight weight);

//std::istream& operator >> (std::istream &i, Weight &weight);

/*
bool operator == (Weight lhs, Weight rhs);
bool operator != (Weight lhs, Weight rhs);

//using namespace std;


Weight operator *(Weight lhs, Weight rhs);

Weight operator /(Weight lhs, Weight rhs);

Weight operator +(Weight lhs, Weight rhs);

Weight operator -(Weight lhs, Weight rhs);

bool operator <(Weight lhs, Weight rhs);
bool operator >(Weight lhs, Weight rhs);
bool operator <=(Weight lhs, Weight rhs);
bool operator >=(Weight lhs, Weight rhs);
*/
  
inline bool operator == (Weight lhs, Weight rhs) { return lhs.weight == rhs.weight; }
inline bool operator != (Weight lhs, Weight rhs) { return lhs.weight != rhs.weight; }

inline Weight operator *(Weight lhs, Weight rhs) {
  Weight result;
  result.weight =  lhs.weight + rhs.weight; 
  return result;
}

inline Weight operator /(Weight lhs, Weight rhs) { 
  Weight result;
  result.weight =  lhs.weight - rhs.weight; 
  return result;
}

#define MUCH_BIGGER_LN 16.f
// represents BIG=10^MUCH_BIGGER_LN - if X > BIG*Y, then X+Y =~ X
// 32 bit FLOAT_TYPE IEEE FLOAT_TYPE has 23 binary digit mantissa, so 
// can represent about 16 base E digits only
// fixme: if you use 64-bit doubles instead of floats, 52 binary digits
// so define as 36.f instead


inline Weight operator +(Weight lhs, Weight rhs) {
  //fixme: below test is needed with glibc without -ffast-math to compute 0+0 properly (?)
  //  if (lhs == 0.0)
  //  return rhs;
#ifdef WEIGHT_CORRECT_ZERO
	if (lhs.isZero())
		return rhs;
	if (rhs.isZero())
		return lhs;
#endif

  FLOAT_TYPE diff = lhs.weight - rhs.weight;
  if ( diff > MUCH_BIGGER_LN )
    return lhs;
  if ( diff < -MUCH_BIGGER_LN )
    return rhs;

  Weight result; 

  if ( diff < 0 ) { // rhs is bigger
	  result.weight = (FLOAT_TYPE)(rhs.weight + log(1 + std::exp(lhs.weight-rhs.weight)));
    return result;
  }
  // lhs is bigger
  result.weight = (FLOAT_TYPE)( lhs.weight + log(1 + std::exp(rhs.weight-lhs.weight)));
  return result;
}

inline Weight operator -(Weight lhs, Weight rhs) {

#ifdef WEIGHT_CORRECT_ZERO
	if (rhs.isZero())
		return lhs;
#endif

  Weight result; 
	FLOAT_TYPE rdiff=rhs.weight-lhs.weight;
  if ( rdiff >= 0 )	   // lhs <= rhs 
	  // clamp to zero as minimum without giving exception (not mathematically correct!)
  {
    //result.weight = -HUGE_FLOAT; // default constructed to this already
    return result;
  }


  if ( rdiff < -MUCH_BIGGER_LN ) // lhs >> rhs
	  return lhs;

  // lhs > rhs
  
  result.weight = (FLOAT_TYPE)(lhs.weight + log(1 - std::exp(rdiff)));
  return result;
}

inline Weight absdiff(Weight lhs, Weight rhs) {
#if 0
	// UNTESTED
	FLOAT_TYPE diff=lhs.weight-rhs.weight;
	if ( diff > MUCH_BIGGER_LN )
		return lhs;
	if ( diff < -MUCH_BIGGER_LN )
		return rhs;
	Weight result;
	if ( diff < 0 )
		result.weight = (FLOAT_TYPE)(rhs.weight + log(1 - std::exp(diff)));
	else
		result.weight = (FLOAT_TYPE)(lhs.weight + log(1 - std::exp(-diff)));
#else
	if (lhs.weight > rhs.weight)
		return lhs-rhs;
	else
		return rhs-lhs;
#endif
}

inline bool operator <(Weight lhs, Weight rhs) { return lhs.weight < rhs.weight; }
inline bool operator >(Weight lhs, Weight rhs) { return lhs.weight > rhs.weight; }
inline bool operator <=(Weight lhs, Weight rhs) { return lhs.weight <= rhs.weight; }
inline bool operator >=(Weight lhs, Weight rhs) { return lhs.weight >= rhs.weight; }

#ifdef _MSC_VER
#pragma warning(pop)

//template<class A,class B> std::basic_ostream<A,B>& operator <<(std::basic_ostream<A,B>& os, std::basic_ostream<A,B>& (*manip)(std::basic_ostream<A,B>&)) { return manip(os);}
// in MSVC++ release optimization mode, manipulators don't work by os << manip, must do manip(os) =(
#endif

template<class A,class B> 
//__declspec(noinline) 
std::basic_ostream<A,B>&
Weight::out_log10(std::basic_ostream<A,B>& os)

 { os.iword(base_index) = LOG10; return os; }

template<class A,class B> std::basic_ostream<A,B>&
Weight::out_ln(std::basic_ostream<A,B>& os) { os.iword(base_index) = LN; return os; }

template<class A,class B> std::basic_ostream<A,B>&
Weight::out_variable(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = VAR; return os; }

template<class A,class B> std::basic_ostream<A,B>&
Weight::out_always_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = ALWAYS_LOG; return os; }

template<class A,class B> std::basic_ostream<A,B>&
Weight::out_always_real(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = ALWAYS_REAL; return os; }

void inline dbgout(std::ostream &o,Weight w) {
    o << Weight::out_always_real << w
#ifdef VERBOSE_DEBUG
      << '=' << Weight::out_always_log << w
#endif
        ;
}

#include <limits>
namespace std {
template <>
class numeric_limits<Weight> {
public:
  static bool has_infinity() { return true; }
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
