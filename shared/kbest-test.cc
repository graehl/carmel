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
/* k Shortest Paths in O(E*log V + L*k*log k) time (L is path length)
   Implemented by Jonathan Graehl (jonathan@graehl.org)
   Following David Eppstein's "Finding the k Shortest Paths" March 31, 1997 draft
   (http://www.ics.uci.edu/~eppstein/
   http://www.ics.uci.edu/~eppstein/pubs/p-kpath.ps)
*/

#define GRAEHL__SINGLE_MAIN
#include "graph.h"
#include "kbest.h"
#include <sstream>
using namespace std;

int main(int argc, char *argv[])
{
  if ( argc < 4 ) {
    cout << "Test of k best paths algorithm.  Supply three integer arguments (source state number, destination state number, number of paths) and a graph to stdin.  A fourth argument will show sidetracks only.  See readme.txt, sample.graph, or ktest.cc for the graph format.\n";
    return 0;
  }
  int k;
  int source, dest;
  istringstream sstr(argv[1]);
  istringstream dstr(argv[2]);
  istringstream kstr(argv[3]);

  if ( !((sstr >> source) && (dstr >> dest) && (kstr >> k)) ) {
    cerr << "Bad argument (should be integer) - aborting.\n";
    return -1;
  }
  Graph graph;

  cin >> graph;
  int n = graph.nStates;
  cerr << "(up to) "<<k<<" best paths from "<<source<<" to "<<dest<<" in graph (#vertices="<<n<<") with (acyclic) "<<countNoCyclePaths<double>(graph, source, dest)<<" such paths:\n";
#define DO(f) BestPathsPrinter<f> printer(cout); bestPaths(graph, source, dest, k, printer);
  if (argc == 4) {
    DO(false);
  } else {
    DO(true);
  }
  return 0;
}
