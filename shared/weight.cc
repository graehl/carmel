/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "weight.h"

Weight Weight::wTmp;

bool operator == (Weight lhs, Weight rhs) { return lhs.weight == rhs.weight; }
bool operator != (Weight lhs, Weight rhs) { return lhs.weight != rhs.weight; }


ostream& operator << (ostream &o, Weight weight)
{
  if ( weight == 0.0 )
    o << 0;
  else if ( weight.weight < 20 && weight.weight > -20 )
    o << pow((double)10, (double)weight.weight);
  else
    o << weight.weight << "log";
  return o;
}

istream& operator >> (istream &i, Weight &weight)
{
  char c;
  float f;
  i >> f;
  if ( i.eof() )
    weight = f;
  else if ( (c = i.get()) == 'l' && i.get() == 'o' && i.get() == 'g' )
    weight.weight = f;
  else {
    i.putback(c);
    weight = f;
  }
  return i;
}

Weight operator *(Weight lhs, Weight rhs) { 
  Weight::wTmp.weight = lhs.weight + rhs.weight; 
  return Weight::wTmp;
}


Weight operator /(Weight lhs, Weight rhs) { 
  Weight::wTmp.weight = lhs.weight - rhs.weight; 
  return Weight::wTmp;
}

Weight operator +(Weight lhs, Weight rhs) {
  if ( lhs == 0.f )
    return rhs;
  if ( rhs == 0.f )
    return lhs;
  if ( rhs.weight > lhs.weight ) {
    Weight::wTmp.weight = rhs.weight + log10(1 + pow((double)10, (double)lhs.weight-rhs.weight));
    return Weight::wTmp;
  }
  Weight::wTmp.weight = lhs.weight + log10(1 + pow((double)10, (double)rhs.weight-lhs.weight));
  return Weight::wTmp;
}

Weight operator -(Weight lhs, Weight rhs) {
  if ( rhs.weight > lhs.weight )	   // clamp to zero as minimum
    return 0.f;
  if ( rhs == 0.f )
    return lhs;
  Weight::wTmp.weight = lhs.weight + log10(1 - pow((double)10, (double)rhs.weight-lhs.weight));
  return Weight::wTmp;
}


bool operator <(Weight lhs, Weight rhs) { return lhs.weight < rhs.weight; }
bool operator >(Weight lhs, Weight rhs) { return lhs.weight > rhs.weight; }
bool operator <=(Weight lhs, Weight rhs) { return lhs.weight <= rhs.weight; }
bool operator >=(Weight lhs, Weight rhs) { return lhs.weight >= rhs.weight; }
