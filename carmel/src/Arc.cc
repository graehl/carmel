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
