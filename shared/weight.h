#ifndef WEIGHT_H 
#define WEIGHT_H 1
#include "config.h"

#ifdef _MSC_VER
#pragma warning(push)
#endif
#include <cmath>
#include <iostream>

#ifdef _MSC_VER
// Microsoft C++: conversion from double to float:
#pragma warning(disable:4244)
#endif

//! warning: unless #ifdef WEIGHT_CORRECT_ZERO
// Weight(0) will may give bad results when computed with, depending on math library behavior
// defining WEIGHT_CORRECT_ZERO will incur a performance penalty

struct Weight {			// capable of representing nonnegative reals 
		// internal implementation note: by their base e logarithm
private:
enum { LOG10=0, LN };
enum { VAR=0, ALWAYS_LOG, ALWAYS_REAL };
// IEE float safe till about 10^38, loses precision earlier (10^32?)
// 32 * ln 10 =~ 73
enum {LN_TILL_UNDERFLOW=73} ;

static const int base_index; // handle to ostream iword for LogBase enum (initialized to 0)
static const int thresh_index; // handle for OutThresh

public:	
  //static const Weight ZERO, INFINITY;
	// linux g++ 3.2 didn't like static self-class member
  static const float HUGE_FLOAT;
  float weight;

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
  float getLog(float base) const {
	  return weight / log(base);
  }
  float getLogImp() const {
	  return weight;
  }
  float getLn() const {
	  return weight;
  }
  float getLog10() const {
	  static const float oo_ln10 = 1./log(10.f);
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
	  return weight == -HUGE_FLOAT;
  }
  bool isPositive() const {
	  return weight > -HUGE_FLOAT;
  }
  void setZero() {
	  weight = -HUGE_FLOAT;
  }
  void setReal(double f) {
	  if (f > 0)
		weight=(float)log(f);
	  else
	    setZero();
  }
  void setLn(float w) {
	  weight=w;
  }
  void setLog10(float w) {
	  static const float ln10 = log(10.f);
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
	  if (isZero())
		return *this;
#endif
    weight += w.weight;
    return *this;
  }
  Weight operator /= (Weight w)
  {
#ifdef WEIGHT_CORRECT_ZERO
	  if (isZero())
		return *this;
#endif
    weight -= w.weight;
    return *this;
  }
  void raisePower(float power) {
	  weight *= power;
  }
  void takeRoot(float nth) {
	  weight /= nth;
  }
  Weight operator ^= (float power) { // raise Weight^power
	  raisePower(power);
	  return *this;
  }

template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& os) const;
template <class charT, class Traits>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& os);

};

inline Weight root(Weight w,float nth) {
	w.takeRoot(nth);
	return w;
}

inline Weight pow(Weight w,float nth) {
	w.raisePower(nth);
	return w;
}


inline Weight operator ^(Weight base,float exponent) {
	Weight result = base;
	result ^= exponent;
	return result;
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
	return std::ios_base::goodbit;
}

template <class charT, class Traits>
std::ios_base::iostate Weight::get_from(std::basic_istream<charT,Traits>& i)
{
  char c;
  double f=0;
  i >> f;
  
  if ( i.fail() ) {
	setZero();
  } else if ( i.eof() ) {
    setReal(f);
  } else if ( (c = i.get()) == 'l' ) {
   char n = i.get();  	
   if ( n == 'n')
    setLn((float)f);
   else if ( n == 'o' && i.get() == 'g' )
    setLog10(f);
   else {
    setZero();
	return std::ios_base::badbit;
   }
  } else {
    i.putback(c);
    setReal(f);
  }
  return std::ios_base::goodbit;
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
// 32 bit float IEEE float has 23 binary digit mantissa, so 
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

  float diff = lhs.weight - rhs.weight;
  if ( diff > MUCH_BIGGER_LN )
    return lhs;
  if ( diff < -MUCH_BIGGER_LN )
    return rhs;

  Weight result; 

  if ( diff < 0 ) { // rhs is bigger
	  result.weight = (float)(rhs.weight + log(1 + std::exp(lhs.weight-rhs.weight)));
    return result;
  }
  // lhs is bigger
  result.weight = (float)( lhs.weight + log(1 + std::exp(rhs.weight-lhs.weight)));
  return result;
}

inline Weight operator -(Weight lhs, Weight rhs) {
  Weight result; 


  if ( lhs.weight <= rhs.weight )	   // lhs <= rhs 
	  // clamp to zero as minimum without giving exception (not mathematically correct!)
  {
    //result.weight = -Weight::HUGE_FLOAT; // default constructed to this already
    return result;
  }

#ifdef WEIGHT_CORRECT_ZERO
	if (rhs.isZero())
		return lhs;
#endif


  if ( lhs.weight - rhs.weight > MUCH_BIGGER_LN ) // lhs >> rhs
	  return lhs;

  // lhs > rhs
  
  result.weight = (float)(lhs.weight + log(1 - std::exp(rhs.weight-lhs.weight)));
  return result;
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



#endif 
