/** \file

    a header for including or a starting point for google code jam main.cc

    #include "codejam.hh"
    struct Case {
      void read();
      void solve();
      void write();
    };

    Cases<Case> cases;


    # generate single source for submission
    forsubmit() {
      ( set -e;
      incl=codejam.hh
      grep -q $incl $1
      (cat codejam.hh; grep -v $incl $1) > submit-$1
      g++ -O -Wall submit-$1 -o $1.exe
      tail submit-$1
      )
    }

    avoid iostreams except for cerr, since stdio is faster

*/

#ifndef CODEJAM_JG_2015_03_29_HH
#define CODEJAM_JG_2015_03_29_HH
#pragma once

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <queue>
#include <deque>
#include <bitset>
#include <iterator>
#include <list>
#include <stack>
#include <map>
#include <set>
#include <functional>
#include <numeric>
#include <utility>
#include <limits>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <gmpxx.h>
typedef mpz_class Big; // swap .get_str() .set_str(str, 10)
inline void gcd(Big &out, Big const& a, Big const& b) {
 mpz_gcd (out.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
 // etc https://gmplib.org/manual/Number-Theoretic-Functions.html
}


using namespace std;

#ifndef NDEBUG
#define O(x) #x << '=' << x
#define CO1(x) cerr << O(x) << '\n'
#define CO2(m, x) cerr << m << ' ' << O(x) << '\n'
#else
#define O(x)
#define CO1(x)
#define CO2(m, x)
#endif

/*******  All Required define Pre-Processors and typedef Constants *******/
#define GETINT(type) readInt<type>()
#define PUTU(x) printf("%u", x);
#define PUTu64(x) printf("%llu", x);
#define GETU readInt<U>()
#define SCC(t) scanf("%c", &t)
#define SCS(t) scanf("%s", t)
#define SCF(t) scanf("%f", &t)
#define SCLF(t) scanf("%lf", &t)
#define MEM(a, b) memset(a, (b), sizeof(a))
#define MEMN(a, n, b) memset(a, (b), n * sizeof(a))
#define FOR(i, j, k, in) for (int i = j; i < k; i += in)
#define RFOR(i, j, k, in) for (int i = j; i >= k; i -= in)
#define REP(i, j) FOR(i, 0, j, 1)
#define RREP(i, j) RFOR(i, j, 0, 1)
#define all(cont) cont.begin(), cont.end()
#define rall(cont) cont.end(), cont.begin()
#define FOREACH(it, l) for (auto it = l.begin(); it != l.end(); it++)
#define IN(A, B, C) assert(B <= A && A <= C)
#define MP make_pair
#define PB push_back
#define INF (int)1e9
#define EPS 1e-9
#define PI 3.1415926535897932384626433832795
#define MOD 1000000007
#define MUSTMOD(x) assert(x >= 0 && x < MOD)
double const pi = acos(-1.0);
typedef long int int32;
typedef unsigned long int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;
typedef int64 i64;
typedef int I;
typedef uint64 u64;
typedef unsigned U;
typedef pair<int, int> PII;
typedef vector<int> VI;
typedef vector<string> VS;
typedef vector<PII> VII;
#define umap unordered_map
#define uset unordered_set
#define mumap unordered_multimap
#define muset unordered_multiset

template <class Int>
Int modplus(Int x, Int y) {
  MUSTMOD(x);
  MUSTMOD(y);
  Int z = x + y;
  return z > MOD ? z - MOD : z;
}

template <class Int>
Int modtimes(Int x, Int y) {
  MUSTMOD(x);
  MUSTMOD(y);
  u64 z = x;
  z *= y;
  return z % MOD;
}

template <class Int>
Int modpow(Int x, U p) {
  MUSTMOD(x);
  assert(p >= 0);
  Int r = 1;
  while (p) {
    if (p & 1) r = modtimes(r, x);
    x = modtimes(x, x);
    p >>= 1;
  }
  return r;
}

/// assign x = min(x,y)
template <class T, class U>
inline void amin(T& x, U y) {
  if (y < x) x = y;
}
template <class T, class U>
inline void amax(T& x, U y) {
  if (x < y) x = y;
}

template <class T>
inline void write(T x) {
  const int maxdigits = 20;
  int i = maxdigits;
  char buf[maxdigits + 1];
  buf[maxdigits] = '\n';
  do {
    buf[--i] = x % 10 + '0';
    x /= 10;
  } while (x);
  do {
    putchar(buf[i]);
  } while (buf[i++] != '\n');
}

template <class T>
inline T readInt() {
  T n = 0, s = 1;
  char p = getchar();
  if (p == '-') s = -1;
  while ((p < '0' || p > '9') && p != EOF && p != '-') p = getchar();
  if (p == '-') s = -1, p = getchar();
  while (p >= '0' && p <= '9') {
    n = n * 10 + (p - '0');
    p = getchar();
  }
  return n * s;
}

#ifndef CASES_DEFAULT_CORES
#ifndef NDEBUG
#define CASES_DEFAULT_CORES 8
#else
#define CASES_DEFAULT_CORES 1
#endif
#endif

inline void casepre(U k) {
  printf("Case #%d: ", k + 1);
}
inline void casepost() {
  putchar('\n');
}

/// only allowed as a global (no memzero for perf)
template <class Case, U MaxCases = 100000>
struct Cases {
  bool single;
  Case cases[MaxCases];
  pthread_t detached_thread;
  pthread_attr_t detached_threadattr;
  Cases() {
    pthread_attr_init(&detached_threadattr);
    pthread_attr_setdetachstate(&detached_threadattr, PTHREAD_CREATE_DETACHED);
  }
  // no dtor: leak is fine
  int main(int argc, char* argv[], U cores = 8) {
    char const* input = "input.txt";
    char const* output = argc == 2 ? 0 : "output.txt";
    if (argc > 1) input = argv[1];
    if (argc > 2) output = argv[2];
    if (input && input[0]) freopen(input, "r", stdin);
    if (output && output[0]) freopen(output, "w", stdout);
    if (argc > 3) cores = atoi(argv[3]);
    U ncases;
    ncases = GETU;
    CO1(ncases);
    if (ncases > MaxCases) abort();
    single = cores <= 1;
    if (!single) {
      sem_init(&nthreads, 0, cores);
      sem_init(&done, 0, 0);
    }
    REP(k, ncases) {
      cases[k].read();
      if (single) {
        cases[k].solve();
        casepre(k);
        cases[k].print();
        casepost();
      } else
        start_thread(k);
    }
    if (!single) {
      REP(k, ncases) sem_wait(&done);
      REP(k, ncases) {
        casepre(k);
        cases[k].print();
        casepost();
      }
    }
    return 0;
  }

  sem_t nthreads, done;
  static void* solve_thread(void* casep) {
    Case* c = (Case*)casep;
    c->solve();
    return 0;
  }
  void start_thread(U casei) {
    assert(casei < MaxCases);
    CO2("solving", casei);
    Case* c = &cases[casei];
    c->casei = casei;
    c->done1 = &done;
    c->done2 = &nthreads;
    sem_wait(&nthreads);
    int ret;
    while ((ret = pthread_create(&detached_thread, &detached_threadattr, &solve_thread, (void*)(size_t) casei)))
      if (ret == EAGAIN)
        sleep(1);
      else {
        fprintf(stderr, "Error creating line_optimize thread\n");
        abort();
      }
  }
};

struct CaseBase {
  void solve() {
    CO2("must override", casei);
    abort();
  }
  sem_t* done1, *done2;
  U casei;
  // must call at end of your solve
  void solved() {
    CO2("solved", casei);
    sem_post(done1);
    sem_post(done2);
  }
};

#define CASES_MAIN_MAX(Case, MAXCASES) \
  static Cases<Case, MAXCASES> gcases; \
  int main(int argc, char* argv[]) { return gcases.main(argc, argv); }
#define CASES_MAIN(Case) static Cases<Case, 1<<20)

#endif
