/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef ARC_H
#define ARC_H 1

#include "weight.h"

struct PathArc {
  const char *in;
  const char *out;
  const char *destState;
  Weight weight;
};

std::ostream & operator << (std::ostream & o, const PathArc &p);

struct Arc {
  int in;
  int out;
  int dest;
  Weight weight;
  int groupId;  
  Arc(int i, int o, int d, Weight w,int g = -1) : 
    in(i), out(o), dest(d), weight(w), groupId(g){}
};

typedef Arc *HalfArc;

void printArc(void *arc, std::ostream &out);

std::ostream & operator << (std::ostream &out,const Arc &a); // Yaser 7-20-2000
						  
struct UnArc {
  int in;
  int out;
  int dest;
  int operator == (const UnArc& r) const {
    return in == r.in && out == r.out && dest == r.dest;
  }
  int hash() const
  {
    return (in * 235479241 + out * 67913 + dest) * 2654435767U;
  }
};

#endif 
