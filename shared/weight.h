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

#include <math.h>
#include <iostream>

struct Weight {			// capable of representing nonnegative reals
  float weight;
  static Weight wTmp;
  Weight(float f) {
    if ( f > 0 )
      weight = log10(f);
    else
      weight = -HUGE_VAL;
  }
  float toFloat() { 
    return pow((double)10, (double)weight);
  }
//  Weight() : weight(-HUGE_VAL) {}
  Weight(){weight=-HUGE_VAL; }
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

bool operator == (Weight lhs, Weight rhs);
bool operator != (Weight lhs, Weight rhs);

ostream& operator << (ostream &o, Weight weight);

istream& operator >> (istream &i, Weight &weight);

Weight operator *(Weight lhs, Weight rhs);

Weight operator /(Weight lhs, Weight rhs);

Weight operator +(Weight lhs, Weight rhs);

Weight operator -(Weight lhs, Weight rhs);

bool operator <(Weight lhs, Weight rhs);
bool operator >(Weight lhs, Weight rhs);
bool operator <=(Weight lhs, Weight rhs);
bool operator >=(Weight lhs, Weight rhs);

  

#endif 
