/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "Arc.h"
using namespace std;

ostream & operator << (ostream & o, const PathArc &p)
{
  o << "(" << p.in << " : " << p.out << " / " << p.weight << " -> " << p.destState << ")";
  return o;
}

ostream & operator << (ostream &out,const Arc &a) { // Yaser 7-20-2000
  out << '(' << a.dest << ' ' << a.in << ' ' << a.out << ' ' << a.weight << ')';
  return(out); 
}
