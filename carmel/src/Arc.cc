#include "config.h"
#include "fst.h"
using namespace std;

ostream & operator << (ostream & o, const PathArc &p)
{
  const WFST *w=p.wfst;
  o << "(" << w->inLetter(p.in) << " : " << w->outLetter(p.out) << " / " << p.weight << " -> " << w->stateName(p.destState) << ")";
  return o;
}

ostream & operator << (ostream &out,const Arc &a) { // Yaser 7-20-2000
  out << '(' << a.dest << ' ' << a.in << ' ' << a.out << ' ' << a.weight << ')';
  return(out);
}
