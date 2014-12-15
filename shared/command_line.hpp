/** \file

    represent and print (shell escaped) command lines.
*/


#ifndef GRAEHL__SHARED__COMMAND_LINE_HPP
#define GRAEHL__SHARED__COMMAND_LINE_HPP

#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/shell_escape.hpp>
#include <sstream>
#include <vector>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <cstring>
#endif

namespace graehl {

template <class O, class Argv>
O& print_command_line(O& out, int argc, Argv const& argv, const char* header = "### COMMAND LINE:\n") {
  if (header) out << header;
  graehl::word_spacer_c<' '> sep;
  for (int i = 0; i < argc; ++i) {
    out << sep;
    out_shell_quote(out, argv[i]);
  }
  if (header) out << '\n';
  return out;
}


template <class Argv>
inline std::string get_command_line(int argc, Argv const& argv, const char* header = "COMMAND LINE:\n") {
  std::ostringstream os;
  print_command_line(os, argc, argv, header);
  return os.str();
}

struct argc_argv : private std::stringbuf {
  typedef char const* arg_t;
  typedef arg_t* argv_t;

  std::vector<char const*> argvptrs;
  int argc() const { return (int)argvptrs.size(); }
  argv_t argv() const { return argc() ? (argv_t) & (argvptrs[0]) : NULL; }
  bool isspace(char c) {
    //    return std::isspace(c);
    return c == ' ' || c == '\n' || c == '\t';
  }

  void throw_escape_eof() const {
    throw std::runtime_error(
        "Error parsing: escape char \\ followed by end of stream (expect some character)");
  }

  // note: str is from stringbuf.
  void parse(const std::string& cmdline, char const* progname = "ARGV") {
    argvptrs.clear();
    argvptrs.push_back(progname);
    str(cmdline + " ");  // we'll need space for terminating final arg.
    char* i = gptr(), * end = egptr();

    char* o = i;
    char terminator;
  next_arg:
    while (i != end && isspace(*i)) ++i;  // [ ]*
    if (i == end) return;

    if (*i == '"' || *i == '\'') {
      terminator = *i;
      ++i;
      argvptrs.push_back(o);
      while (i != end) {
        if (*i == '\\') {
          ++i;
          if (i == end) throw_escape_eof();
        } else if (*i == terminator) {
          *o++ = 0;
          ++i;
          goto next_arg;
        }
        *o++ = *i++;
      }
    } else {
      argvptrs.push_back(o);
      while (i != end) {
        if (*i == '\\') {
          ++i;
          if (i == end) throw_escape_eof();
        } else if (isspace(*i)) {
          *o++ = 0;
          ++i;
          goto next_arg;
        }
        *o++ = *i++;
      }
    }
    *o++ = 0;
  }
  argc_argv() : argvptrs() {}
  explicit argc_argv(const std::string& cmdline, char const* progname = "ARGV") { parse(cmdline, progname); }
};

template <class O, class Argv>
void print_command_header(O& o, int argc, Argv const& argv) {
  print_command_line(o, argc, argv);
  print_current_dir(o);
  o << '\n';
}

#ifdef GRAEHL_TEST
char const* test_strs[] = {"ARGV", "ba", "a", "b c", "d", " e f ", "123", 0};

BOOST_AUTO_TEST_CASE(TEST_command_line) {
  using namespace std;
  {
    string opts = "ba a \"b c\" 'd' ' e f ' 123";
    argc_argv args(opts);
    BOOST_CHECK_EQUAL(args.argc(), 7);
    for (int i = 1; i < args.argc(); ++i) {
      CHECK_EQUAL_STRING(test_strs[i], args.argv()[i]);
    }
  }
  {
    string opts = " ba a \"\\b c\" 'd' ' e f '123 ";
    argc_argv args(opts);
    BOOST_CHECK_EQUAL(args.argc(), 7);
    for (int i = 1; i < args.argc(); ++i) {
      CHECK_EQUAL_STRING(test_strs[i], args.argv()[i]);
    }
  }
  {
    argc_argv args("");
    BOOST_CHECK_EQUAL(args.argc(), 1);
  }
}
#endif

}  // graehl

#endif
