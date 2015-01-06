// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
