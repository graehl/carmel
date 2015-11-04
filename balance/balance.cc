#include "balance.lex.hh"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

void err(char const* msg) {
  cerr << msg << '\n';
  abort();
}

void run(istream& in, char const* name) {
  cerr << name << " ...\n";
  yyFlexLexer l(&in, &cerr);
  while (in && l.yylex()) {
    cout << string(l.YYText(), l.YYLeng()) << "\n";
  }
  cout << '\n';
}

void run(char const* name) {
  ifstream in(name);
  if (!in) err(name);
  run(in, name);
}

int main(int argc, char* argv[]) {
  if (argc < 2)
    run(cin, "[STDIN]");
  else
    for (int i = 1; i < argc; ++i) run(argv[i]);
  return 0;
}
