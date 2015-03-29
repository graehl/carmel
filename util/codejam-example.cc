/** \file

 .
*/

#include "codejam.hh"

struct Times2 : CaseBase {
  U in, exponent;
  u64 prod, pow;
  void read() {
    in = GETU;
    exponent = GETU;
  }
  void print() {
    PUTU(in);
    putchar(' ');
    PUTU(exponent);
    putchar(' ');
    //PUTu64(prod);
    putchar(' ');
    //PUTu64(pow);
  }
  void solve() {
    //prod = modtimes(in, exponent);
    //pow = modpow(in, exponent);
  }
};

CASES_MAIN_MAX(Times2, 2)
