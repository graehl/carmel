/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "weight.h"

std::ostream& operator << (std::ostream &o, Weight weight)
{
  if ( weight == 0.0 )
    o << 0;
  else if ( weight.weight < 20 && weight.weight > -20 )
    o << pow((double)10, (double)weight.weight);
  else
    o << weight.weight << "log";
  return o;
}

std::istream& operator >> (std::istream &i, Weight &weight)
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
