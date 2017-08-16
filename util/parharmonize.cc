/** \file

    identify numbers and transliterations in parallel (line by line) data.
TODO: implement :)
*/

#define GRAEHL__SINGLE_MAIN 1

#include <graehl/shared/configure_program_options.hpp>
#include <graehl/shared/fileargs.hpp>
#include <cassert>

typedef long int Long;

using namespace graehl;
using namespace std;


struct Entities {
  istream_arg in;
  std::string line;
  vector<string> ents;
  vector<string> betweens;  // #ents + 1
  static bool entchar(char c) { return c >= '0' && c <= '9'; }
  bool readline() {
    ents.clear();
    betweens.clear();
    if (!getline(*in, line)) return false;
    typedef char const* P;
    typedef unsigned I;
    I len = line.size();
    if (!len) {
      betweens.emplace_back();
      return;
    }
    P i = &line[0], e = i + len, b = i;
    for (; i != e; ++i) {
      if (entchar(*i)) betweens.emplace_back(b, i);
    }
    bool ent = true;
    for (;;) {
      bool end = ++i == e;
      if (end || entchar(*i) != ent) {
        if (ent) {
          ents.emplace_back(b, i);
          if (end) {
            betweens.emplace_back();
            break;
          }
        } else {
          betweens.emplace_back(b, i);
          if (end) break;
        }
      }
    }
    assert(betweens.size() == ents.size() + 1);
    return true;
  }
  void write(std::ostream& o) const {
    unsigned i = 0, n = ents.size();
    assert(betweens.size() == n + 1);
    o << betweens[i];
    while (i != n) {
      o << ents[i];
      o << betweens[++i];
    }
    o << '\n';
  }
};

typedef pair<Long, Long> Interval;
constexpr Long kLongMax = (Long)-1 / 2;

inline bool inside(Interval i, Long x) {
  return x >= i.first && x < i.second;
}

inline bool empty(Interval i) {
  return i.first >= i.second;
}

///
struct Offset {
  Long off = 0;
  Interval fromi{0, 0};
  bool enabled() const { return !empty(fromi); }
  bool match(Long from, Long to) const { return inside(fromi, from) && from + off == to; }
  template <class Config>
  void configure(Config& c) {
    c("off", &off)("offset (2nd-1st) that's added to first to match second").defaulted();
    c("from-at-least", &fromi.first)("1st must be >= this or offset isn't allowed");
    c("from-under", &fromi.second)("1st must be < this or offset isn't allowed");
    c.is("Offset");
    c("year numbers (and month numbers / day numbers?) may offer by a constant off e.g. "
      "https://en.wikipedia.org/wiki/Buddhist_calendar has recent years 'too large' by 543");
  }
};

struct Within {
  Interval d{kLongMax, 0};
  bool enabled() const { return !empty(fromi); }
  bool match(Long from, Long to) const {
    Long diff = to - from;
    return diff >= d.first && diff <= d.second;
  }
  template <class Config>
  void configure(Config& c) {
    c("dmin", &d.min)("(2nd-1st) must be at least this. special case: if min>max then min is set to = -max").defaulted();
    c("dmax", &d.second))("(2nd-1st) must be at most this").defaulted();
    c.is("Within");
  }
  typedef Within configure_validate;
  void validate() {
    if (d.first > d.second) d.first = -d.second;
  }
};

struct Harmonize {
  Entities e[2];
  Offset offset;
  Within within;
  int verbose = 0;
  template <class Config>
  void configure(Config& c) {
    c("sfile", &e[0].in)('s')("file 1").positional();
    c("tfile", &e[1].in)('t')("file 2").positional();
    c("verbose", &verbose)('v').flag()("verbose [N]").defaulted();
    c("offset", &offset);
    c.is("Harmonize");
    c("tfile is minimally adjusted to match sfile (in digit strings only so far), or lines are discarded "
      "entirely if distance is too far");
  }
  bool run() {
    Long lineno = 0;
    for (;;) {
      ++lineno;
      bool r1 = e[0].readline(), r2 = e[1].readline();
      if (r1 != r2) {
        cerr << "ERROR: line #" << lineno << " not in both files: " << r1 << " in " << e[0].in.name()
             << " vs " << r2 << " in " << e[1].in.name() << '\n';
        return false;
      }

      if (!r1) break;
    }
    return true;
  }
};

Harmonize harmonize;

int main(int argc, char* argv[]) {
  configure::program_options_exec_new exec(configure::warn_consumer(), "SAMPLE");
  graehl::warn_consumer to_cerr;
  try {
    configure::program_options_action(exec, configure::init_config(), &harmonize, to_cerr);
    exec.p->set_main_argv(argc, argv);
    if (program_options_maybe_help(cout, exec, &harmonize, to_cerr)) return 0;
    configure::program_options_action(exec, configure::store_config(), &harmonize, to_cerr);
    configure::validate_stored(&harmonize, to_cerr);
    cout << "\neffective:\n";
    configure::show_effective(cout, &harmonize);
  }

  return 0;
}
