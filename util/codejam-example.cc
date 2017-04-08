#define MAXCASES 100
#include "codejam.hh"

struct Case : CaseBase {
  typedef map<u64, U> Runs;
  Runs runs;
  u64 N, K;
  u64 floorhalf;
  u64 ceilhalf;
  u64 take() {
    assert(!runs.empty());
    auto i = runs.end();
    --i;
    u64 k = i->first;
    U& n = i->second;
    if (!--n) runs.erase(i);
    return k;
  }
  void enter() {
    u64 last = take();
    assert(last);
    --last;
    floorhalf = last / 2;
    ceilhalf = last - floorhalf;
    add(floorhalf);
    add(ceilhalf);
  }
  void add(u64 x) {
    if (x) ++runs[x];
  }

  void read() {
    N = GETu64;
    K = GETu64;
    assert(N);
    assert(K);
    assert(K <= N);
    runs.clear();
    runs[N] = 1;
  }
  void print() {
    putU(ceilhalf);
    putsp();
    putU(floorhalf);
  }
  void show1() {
  }
  void solve() {
    for (u64 i = 0; i < K; ++i) enter();
  }
};

CASES_MAIN(Case)
