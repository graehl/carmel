/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "weight.h"
const float Weight::HUGE_FLOAT = (float)HUGE_VAL;
// ~6 * ln 10 
#define LN_6_DECIMAL_DIGITS 13

std::ostream& operator << (std::ostream &o, Weight weight)
{
  if ( weight == 0.0 )
    o << 0;
  else if ( weight.weight < LN_6_DECIMAL_DIGITS && weight.weight > -LN_6_DECIMAL_DIGITS )
    o << exp(weight.weight);
  else
    o << weight.weight << "ln";
  return o;
}

std::istream& operator >> (std::istream &i, Weight &weight)
{
  static const float ln10 = log(10.f);

  char c;
  double f;
  i >> f;
  if ( i.eof() )
    weight = f;
  else if ( (c = i.get()) == 'l' ) {
   char n = i.get();  	
   if ( n == 'n')
    weight.weight = (float)f;
   else if ( n == 'o' && i.get() == 'g' )
    weight.weight = (float)f * ln10;
   else
    weight = 0;
  } else {
    i.putback(c);
    weight = f;
  }
  return i;
}
