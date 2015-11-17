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
// some details of writing Dot files for AT&T GraphViz
#ifndef GRAPHVIZ_HPP
#define GRAPHVIZ_HPP

#include <iostream>
#include <string>
#include <graehl/shared/quote.hpp>

namespace graehl {

static char const* GRAPHVIZ_DEFAULT_PRELUDE="node [shape=ellipse,width=.1,height=.1];\n edge [arrowhead=none];\n graph [center=1];\n";

//ordering=out;\n concentrate=0;\n\n ranksep=.3;

template <class L>
struct DefaultNodeLabeler {
  typedef L Label;
  void print(std::ostream &o, const Label &l) {
    o << "label=";
    out_ensure_quote(o, l);
  }
};

struct GraphvizPrinter {
  unsigned next_node;
  std::ostream &o;
  std::string graphname, pre;
  unsigned graph_no;
  GraphvizPrinter(std::ostream &o_, const std::string &prelude_, const char *graphname_="graph") : o(o_), graphname(graphname_), pre(prelude_), graph_no(0) {
    prelude();
  }
  ~GraphvizPrinter() {
    coda();
  }
  void prelude(const char *graphname_ = NULL) {
    if (!graphname_)
      graphname_ = graphname.c_str();
    o << "digraph ";
    out_quote(o, graphname_);
    o << graph_no++;
    o << "{\n";
    o << ' ' << GRAPHVIZ_DEFAULT_PRELUDE << "\n";
    o << ' ' << pre << "\n";
    next_node = 0;
  }
  void caption(const std::string &caption)
  {
    //        o << " graph [labelfontsize=24,label=";
    o << " label=";
    out_always_quote(o, caption);
    o << '\n';
    //        o << "]\n";
  }
  void coda() {
    o << "}\n\n";
  }
  void next() {
    coda();
    prelude();
  }
};

}

#endif
