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
#include <iostream>
#include <iterator>
#include <sstream>
#define MAXLENGTH 70


template <class I, class O>
O &cpp_escape_string(O &out, I begin, I end, unsigned wrap_column = (unsigned)-1)
{
  unsigned column = 0;
  out << '"';
  for (; begin!=end; ++begin) {
    char c=*begin;
    if ( column >= wrap_column ) {
      out << "\"\n\"";
      column = 0;
    }
    column += 2;
    switch (c) {
      case '\n':
        out << "\\n";
        break;
      case '\t':
        out << "\\t";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\'':
        out << "\\\'";
        break;
      case '\"':
        out << "\\\"";
        break;
      default:
        out << c;
        --column;
    }

  }
  out << '"';
  return out;
}

template <class I, class O>
O &cpp_escape_string(O &out, I &in, unsigned wrap_column = (unsigned)-1)
{
  in >> std::noskipws;
  typedef std::istream_iterator<char> Isi;
  return cpp_escape_string(out, Isi(in), Isi(), wrap_column);
}

void usage(char *argv0)
{
  std::cerr << argv0 << " [variable_name] [max_line_length]\n\n";
  std::cerr << "Provide text on stdin and I'll give you on stdout a C character constant variable,\nwhose name you can specify as the first command-line argument.\n(Second argument will break the text into line-size chunks)";
}

int main(int argc, char *argv[])
{
  using namespace std;
  char const *textvar="the_text";
  unsigned max_line_length = MAXLENGTH;
  if (argc > 1) {
    if (argv[1][0]=='-') {
      usage(argv[0]);
      return -1;
    } else {
      textvar = argv[1];
    }
  }
  if (argc > 2) {
    if ((istringstream(argv[2]) >> max_line_length).bad()) {
      usage(argv[0]);
      cerr << "\nExpected an integer max_line_length.\n";
      return -1;
    }
  }
  cin.tie(0);
  cout << "const char *" << textvar << " =\n" ;
  cpp_escape_string(cout, cin, max_line_length);
  cout << "\n;\n";
  return 0;
}
