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

#include <cmath>
#include <iostream>

struct Weight {			// capable of representing nonnegative reals 
  static const float HUGE_FLOAT;
	// internal implementation note: by their base 10 logarithm (subject to change, of course)
  float weight;
  static Weight result;
  Weight(double f) {
    if ( f > 0 )
      weight = (float)log(f);
    else
      weight = -HUGE_FLOAT;
  }

  double toFloat() { 
    return exp(weight);
  }
//  Weight() : weight(-HUGE_FLOAT) {}
  Weight() { weight=-HUGE_FLOAT; }
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
};

const float Weight::HUGE_FLOAT = (float)HUGE_VAL;

std::ostream& operator << (std::ostream &o, Weight weight);

std::istream& operator >> (std::istream &i, Weight &weight);

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

#endif 
