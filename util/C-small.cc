/**

 */

#define MAXCASES 100
#include "codejam.hh"

struct Case : CaseBase {
  I y;
  void read() {
    y = gety();
  }
  I gety() {
    return -1;
  }
  void print() {
    PUTU(y);
  }
  void show1() { cerr << " => " << y; }
  void solve() {}
};

CASES_MAIN(Case)
