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
#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#ifndef MAXCASES
#define MAXCASES 1000
#endif

using namespace std;

int verbose = 1;
#ifdef NDEBUG
#define IFVERBOSE(x)
#else
#define IFVERBOSE(x) \
  do {               \
    if (verbose) x;  \
  } while (0)
#endif

#define O(x) ' ' << #x << '=' << x
#define CO(a) IFVERBOSE(cerr << O(a) << '\n');
#define COO(a, b) IFVERBOSE(cerr << a << ' ' << b << '\n');
#define CO2(m, a) IFVERBOSE(cerr << m << O(a) << '\n');
#define CO3(m, a, b) IFVERBOSE(cerr << m << O(a) << O(b) << '\n');
#define CO4(m, a, b, c) IFVERBOSE(cerr << m << O(a) << O(b) << O(c) << '\n');
#define CO5(m, a, b, c, d) IFVERBOSE(cerr << m << O(a) << O(b) << O(c) << O(d) << '\n');
#define CO6(m, a, b, c, d, e) IFVERBOSE(cerr << m << O(a) << O(b) << O(c) << O(d) << O(e) << '\n');
/// enabled always, unlike assert(a == b)
#define MUSTEQ(a, b) musteq(a, b, #a, #b)

/*******  All Required define Pre-Processors and typedef Constants *******/
#define GETINT(type) readInt<type>()
#define GET0 getdigit()
inline void putsp() {
  putchar(' ');
}
inline void put0(char x) {
  assert(x >= 0);
  assert(x < 16);
  putchar('0' + x);
}
#define PUTU(x) putU((U)x);
#define PUTu64(x) putU((u64)x);
#define PUTIMPOSSIBLE printf("IMPOSSIBLE");
template <class U>
inline void putU(U x) {
  if (x == (U)-1) PUTIMPOSSIBLE else printf("%u", x);
}
#define GETI readInt<I>()
#define GETU readInt<U>()
#define GETu64 readInt<u64>()
#define SCC(t) scanf("%c", &t)
#define SCS(t) scanf("%s", t)
#define SCF(t) scanf("%f", &t)
#define SCLF(t) scanf("%lf", &t)
#define MEM(a, b) memset(a, (b), sizeof(a))
#define MEMN(a, n, b) memset(a, (b), n * sizeof(a))
#define FOR(i, j, k, in) for (int i = j; i < k; i += in)
#define RFOR(i, j, k, in) for (int i = j; i >= k; i -= in)
#define REP(i, n) FOR (i, 0, n, 1)
#define REPTO(i, n) for (int i = 0; i <= n; ++i)
#define RREP(i, high) RFOR (i, high, 0, 1)
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
#ifdef NDEBUG
typedef char i8;
#else
typedef short i8;
#endif
typedef unsigned char u8;
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
typedef vector<U> VU;
typedef vector<string> VS;
typedef vector<PII> VII;
#define umap unordered_map
#define uset unordered_set
#define mumap unordered_multimap
#define muset unordered_multiset

template <class U>
void setImpossible(U& x) {
  x = (U)-1;
}

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

template <class T>
inline T& min3(T& a, T& b, T& c) {
  if (a < b)
    return a < c ? a : c;
  else
    return b < c ? b : c;
}

template <class T>
inline T& max3(T& a, T& b, T& c) {
  if (a > b)
    return a > c ? a : c;
  else
    return b > c ? b : c;
}

/// assign x = min(x,y)
template <class T, class U>
inline void amin(T& bound, U y) {
  if (y < bound) {
    CO3("new amin bound <= y", bound, y);
    bound = y;
  }
}

template <class T, class U>
inline void amax(T& bound, U y) {
  if (bound < y) {
    CO3("new amax bound <= y", bound, y);
    bound = y;
  }
}

void yesno(bool x) {
  printf(x ? "YES" : "NO");
}

template <class A, class B>
void musteq(A const& a, B const& b, char const* an, char const* bn) {
  IFVERBOSE(cerr << "Should agree: " << an << ":" << a << " ==? " << bn << ":" << b << '\n');
  if (a != b) {
    cerr << "ERROR: did not agree: " << an;
    if (!verbose) cerr << ":" << a;
    cerr << " != " << bn;
    if (!verbose) cerr << ":" << b;
    cerr << '\n';
    abort();
  }
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

inline char getbyte() {
  int c = getchar();
  if (c == EOF) abort();
  return c;
}

template <class T>
inline T readInt() {
  T n = 0, s = 1;
  int p = getchar();
  if (p == '-') s = -1;
  while ((p < '0' || p > '9') && p != EOF && p != '-') p = getchar();
  if (p == '-') s = -1, p = getchar();
  while (p >= '0' && p <= '9') {
    n = n * 10 + (p - '0');
    p = getchar();
  }
  return n * s;
}

inline void casepre(U k) {
  cerr << '.';
  printf("Case #%d: ", k + 1);
}
inline void casepost() {
  putchar('\n');
}
char digit0(int x) {
  return '0' + x;
}

#ifndef __APPLE__
pthread_t detached_thread;
pthread_attr_t detached_threadattr;
sem_t sem_nthreads, sem_done;

template <class Case>
void* solve_thread(void* casep) {
  Case* c = (Case*)casep;
  c->solve();
  cerr << '.';
  sem_post(&sem_nthreads);
  sem_post(&sem_done);
  return 0;
}
#endif

U ncases;
bool singlethread;

void expect_newline() {
  for (;;) {
    int c = getchar();
    if (c == '\n') return;
    if (c == EOF) return;
    if (!isspace(c)) {
      cerr << "error in read() - got nonspaces at end of line: '" << (char)c << "'\n";
      abort();
    }
  }
}

void expect_eof() {
  for (;;) {
    int c = getchar();
    if (c == EOF) return;
    if (!isspace(c)) {
      cerr << "error in read() - got nonspaces at end of file: '" << (char)c << "'\n";
      abort();
    }
  }
}

char const* basename_end(char const* s) {
  char const* r = 0;
  for (; *s; ++s)
    if (*s == '.') r = s;
  return r ? r : s;
}

/// only allowed as a global (no memzero for perf)
template <class Case>
int cases_main(Case* cases, int argc, char* argv[], int cores, bool verify_newline = true) {
#ifndef __APPLE__
  pthread_attr_init(&detached_threadattr);
  pthread_attr_setdetachstate(&detached_threadattr, PTHREAD_CREATE_DETACHED);
#endif
  char const* input = "input.txt";
  if (argc > 1 && *argv[1]) input = argv[1];
  std::string bufoutput;
  char const* output = 0;
  if (argc > 2 && *argv[2]) output = argv[2];
  if (!output) {
    bufoutput.assign(input, basename_end(input));
    bufoutput += ".out";
    output = bufoutput.c_str();
  } else if (output[0] == '-' && !output[1])
    output = 0;
  if (input) freopen(input, "r", stdin);
  if (output) freopen(output, "w", stdout);
  if (argc > 3) cores = atoi(argv[3]);
  if (argc > 4) verbose = atoi(argv[4]);
  ncases = GETU;
  CO(ncases);
  if (ncases > MAXCASES) abort();
#ifdef __APPLE__
  singlethread = true;
#else
  singlethread = cores <= 1;
  if (!singlethread) {
    sem_init(&sem_nthreads, 0, cores);
    sem_init(&sem_done, 0, 0);
  }
#endif
  REP (k, ncases) {
    cases[k].read();
    if (!k && verbose) {
      cerr << "Showing case #1: ";
      cases[k].show1();
      cerr << '\n';
    }
#ifndef __APPLE__
    if (!singlethread) {
      verbose = 0;
      assert(k < MAXCASES);
      Case& c = cases[k];
      c.casei = k;
      sem_wait(&sem_nthreads);
      int ret;
      while ((ret = pthread_create(&detached_thread, &detached_threadattr, &solve_thread<Case>, &c)))
        if (ret == EAGAIN)
          sleep(1);
        else {
          fprintf(stderr, "Error creating solve() thread\n");
          abort();
        }
      cerr << "started solve thread case #" << k + 1 << '\n';
    }
#endif
  }
  expect_eof();
#ifndef __APPLE__
  if (!singlethread) REP (k, ncases)
      sem_wait(&sem_done);
#endif
  REP (k, ncases) {
    if (singlethread) cases[k].solve();
    casepre(k);
    cases[k].print();
    casepost();
  }
  char const* outputname = output ? output : "STDOUT";
  cerr << "done. solved all " << ncases << " - output is in " << outputname << '\n';
  return 0;
}

/// global Cases array will be 0 init so don't worry
struct CaseBase {
  static void init() {}
  bool firstprint;
  void space() {
    if (firstprint)
      firstprint = false;
    else
      putchar(' ');
  }
  void show1() { cerr << "[unimplemented:showo()]"; }
  void solve() {
    CO2("must override", casei);
    abort();
  }
  std::ostream& cpre() { return cerr << "(#" << casei + 1 << "):"; }
  U casei;
};

// nonwhitespace printable
inline char getC() {
  for (;;) {
    int c = getchar();
    assert(c != EOF);
    assert(c != 127);
    if (c == EOF) abort();
    if (c > ' ') return c;
  }
}

inline bool getbool(bool& to, char t = '1', char f = '0') {
  char c = getbyte();
  if (c == t)
    to = true;
  else if (c == f)
    to = false;
  else
    return false;
  return true;
}

inline U getdigit() {
  char c = getC();
  assert(c >= '0');
  assert(c <= '9');
  return c - '0';
}

template <class T>
T* array0(vector<T>& v) {
  return &*v.begin();
}

template <class T>
T* arrayn(vector<T>& v) {
  return &*v.end();
}

template <class It>
struct Showr {
  It a, b;
  Showr(It a, It b) : a(a), b(b) {}
  friend inline std::ostream& operator<<(std::ostream& out, Showr const& self) {
    self.print(out);
    return out;
  }
  void print(std::ostream& out) const {
    char const* comma = "";
    for (It i = a; i < b; ++i) {
      out << comma << *i;
      comma = ",";
    }
  }
};
template <class X>
Showr<typename X::const_iterator> showr(X const& x) {
  return Showr<typename X::const_iterator>(x.begin(), x.end());
}
template <class X>
Showr<X> showr(X a, X b) {
  return Showr<X>(a, b);
}
template <class X>
Showr<X> shown(X a, U n) {
  return Showr<X>(a, a + n);
}

template <class X>
void reversec(X& x) {
  std::reverse(x.begin(), x.end());
}

template <class V>
V sum(V const* a, V const* b) {
  V v = 0;
  for (; a != b; ++a) v += *a;
  return v;
}

template <class V>
V sumn(V const* a, U n) {
  return sum(a, a + n);
}

#ifndef CASES_DEFAULT_CORES
#ifdef NDEBUG
#define CASES_DEFAULT_CORES 1
#else
#define CASES_DEFAULT_CORES 1
#endif
#endif
#define CASES_MAIN(Problem)                                             \
  static Problem cases[MAXCASES];                                       \
  int main(int argc, char* argv[]) {                                    \
    Problem::init();                                                    \
    return cases_main<Problem>(cases, argc, argv, CASES_DEFAULT_CORES); \
  }

#endif
