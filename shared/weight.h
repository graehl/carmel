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

const float HUGE_FLOAT = (float)HUGE_VAL;

struct Weight {			// capable of representing nonnegative reals 
	// internal implementation note: by their base 10 logarithm (subject to change, of course)
  float weight;
  static Weight result;
  Weight(double f) {
    if ( f > 0 )
      weight = (float)log10(f);
    else
      weight = -HUGE_FLOAT;
  }
  double toFloat() { 
    return pow((double)10, (double)weight);
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

#define MUCH_BIGGER_LOG 10.f
// represents 10^10 - if X > 10^10*Y, then X+Y =~ X - float can represent about 7-8 decimal digits only

inline Weight operator +(Weight lhs, Weight rhs) {
	float diff = lhs.weight - rhs.weight;
	if ( diff > MUCH_BIGGER_LOG )
	  return lhs;
	if ( diff < MUCH_BIGGER_LOG )
	  return rhs;

  Weight result; 

  if ( diff < 0 ) { // rhs is bigger
    result.weight = (float)(rhs.weight + log10(1 + pow((double)10, (double)lhs.weight-rhs.weight)));
    return result;
  }
  // lhs is bigger
  result.weight = (float)( lhs.weight + log10(1 + pow((double)10, (double)rhs.weight-lhs.weight)));
  return result;
}

inline Weight operator -(Weight lhs, Weight rhs) {
  if ( lhs.weight < rhs.weight )	   // lhs < rhs 
	  // clamp to zero as minimum without giving exception (not mathematically correct!)
    return 0.f;

  if ( lhs.weight - rhs.weight > MUCH_BIGGER_LOG ) // lhs >> rhs
	  return lhs;

  // lhs > rhs
  Weight result; 
  result.weight = (float)(lhs.weight + log10(1 - pow((double)10, (double)rhs.weight-lhs.weight)));
  return result;
}

inline bool operator <(Weight lhs, Weight rhs) { return lhs.weight < rhs.weight; }
inline bool operator >(Weight lhs, Weight rhs) { return lhs.weight > rhs.weight; }
inline bool operator <=(Weight lhs, Weight rhs) { return lhs.weight <= rhs.weight; }
inline bool operator >=(Weight lhs, Weight rhs) { return lhs.weight >= rhs.weight; }

#endif 
