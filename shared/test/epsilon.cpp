#include <graehl/shared/epsilon.hpp>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>

using namespace std;
using namespace graehl;

template <class Float>
void showApart(Float a, Float b) {
  cout<<"a="<<a<<" b="<<b<<" ieee_apart(a,b)="<<ieee_apart(a,b)<<"\n";
}


int main(int argc, char *argv[])
{
  if (argc!=3) {
    cerr<<"given two floating point arguments; prints # of representable float, double between them\n";
    return -1;
  }
  double a=atof(argv[1]);
  double b=atof(argv[2]);
  showApart((float)a,(float)b);
  showApart(a,b);
}
