/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef WEIGHT_H 
#define WEIGHT_H 1

#ifdef _MSC_VER
#pragma warning(push)
#endif
#include <cmath>
#include <iostream>

#ifdef _MSC_VER
// Microsoft C++: conversion from double to float:
#pragma warning(disable:4244)
#endif

struct Weight {			// capable of representing nonnegative reals 
		// internal implementation note: by their base e logarithm
private:
enum { LOG10, LN } LogBase;
enum { VAR, ALWAYS_LOG, ALWAYS_REAL } OutThresh;
// IEE float safe till about 10^38, loses precision earlier (10^32?)
// 32 * ln 10 =~ 73
enum {LN_TILL_UNDERFLOW=73} ;

static const int base_index; // handle to ostream iword for LogBase enum (initialized to 0)
static const int thresh_index; // handle for OutThresh

public:	
  static const float HUGE_FLOAT;
  float weight;

// output format manipulators: cout << Weight::out_log10;

template<class A,class B> static inline std::basic_ostream<A,B>&
out_log10(std::basic_ostream<A,B>& os) { os.iword(base_index) = LOG10; }

template<class A,class B> static inline std::basic_ostream<A,B>&
out_ln(std::basic_ostream<A,B>& os) { os.iword(base_index) = LN; }

template<class A,class B> static inline std::basic_ostream<A,B>&
out_variable(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = VAR; }

template<class A,class B> static inline std::basic_ostream<A,B>&
out_always_log(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = ALWAYS_LOG; }

template<class A,class B> static inline std::basic_ostream<A,B>&
out_always_real(std::basic_ostream<A,B>& os) { os.iword(thresh_index) = ALWAYS_REAL; }

  static Weight result;
  // default = operator:
  Weight(double f) {
	setReal(f);
  }

  //double toFloat() const { 
    //return getReal();
  //}

  double getReal() const {
	  return exp(weight);
  }
  float getLn() const {
	  return weight;
  }
  float getLog10() const {
	  static const float oo_ln10 = 1./log(10.f);
	  return oo_ln10 * weight;
  }
  bool fitsInReal() const {
	return (getLn() < LN_TILL_UNDERFLOW && getLn() > -LN_TILL_UNDERFLOW);
  }
  bool isZero() const {
	  return weight == -HUGE_FLOAT;
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
    weight += w.weight;
    return *this;
  }
  Weight operator /= (Weight w)
  {
    weight -= w.weight;
    return *this;
  }

template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& os) const;
template <class charT, class Traits>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& os);

};

template <class charT, class Traits>
std::ios_base::iostate Weight::print_on(std::basic_ostream<charT,Traits>& o) const
{
	if ( (o.iword(thresh_index) == VAR && fitsInReal()) || o.iword(thresh_index) == ALWAYS_REAL ) {
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
  double f;
  i >> f;
  if ( i.eof() )
    setReal(f);
  else if ( (c = i.get()) == 'l' ) {
   char n = i.get();  	
   if ( n == 'n')
    setLn((float)f);
   else if ( n == 'o' && i.get() == 'g' )
    setLog10(f);
   else {
    setReal(0);
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
  float diff = lhs.weight - rhs.weight;
  if ( diff > MUCH_BIGGER_LN )
    return lhs;
  if ( diff < -MUCH_BIGGER_LN )
    return rhs;

  Weight result; 

  if ( diff < 0 ) { // rhs is bigger
    result.weight = (float)(rhs.weight + log(1 + exp(lhs.weight-rhs.weight)));
    return result;
  }
  // lhs is bigger
  result.weight = (float)( lhs.weight + log(1 + exp(rhs.weight-lhs.weight)));
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
  
  if ( lhs.weight - rhs.weight > MUCH_BIGGER_LN ) // lhs >> rhs
	  return lhs;

  // lhs > rhs
  
  result.weight = (float)(lhs.weight + log(1 - exp(rhs.weight-lhs.weight)));
  return result;
}

inline bool operator <(Weight lhs, Weight rhs) { return lhs.weight < rhs.weight; }
inline bool operator >(Weight lhs, Weight rhs) { return lhs.weight > rhs.weight; }
inline bool operator <=(Weight lhs, Weight rhs) { return lhs.weight <= rhs.weight; }
inline bool operator >=(Weight lhs, Weight rhs) { return lhs.weight >= rhs.weight; }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif 
