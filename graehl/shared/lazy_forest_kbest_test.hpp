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
/** \file

   test cases for lazy_forest_kbest.
*/

#ifndef GRAEHL_SHARED__LAZY_FOREST_KBEST_TEST__HPP
#define GRAEHL_SHARED__LAZY_FOREST_KBEST_TEST__HPP
#pragma once

#if defined(LAZY_FOREST_EXAMPLES)
# if USE_DEBUGPRINT
#  include <graehl/shared/info_debug.hpp>
//# include "default_print.hpp"
//FIXME: doesn't work
#  include <graehl/shared/debugprint.hpp>
# endif
# include <iostream>
# include <string>
# include <sstream>
# include <cmath>
#endif

#include <graehl/shared/lazy_forest_kbest.hpp>

namespace graehl {

#ifdef LAZY_FOREST_EXAMPLES

namespace lazy_forest_kbest_example {

using namespace std;
struct Result {
  struct Factory
  {
    typedef Result *derivation_type;
// static derivation_type NONE, PENDING;
// enum { NONE=0, PENDING=1 };
    static derivation_type NONE() { return (derivation_type)0;}
    static derivation_type PENDING() { return (derivation_type)1;}
    derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child, lazy_kbest_index_type changed_child_index)
    {
      return new Result(prototype, old_child, new_child, changed_child_index);
    }
  };


  Result *child[2];
  string rule;
  string history;
  float cost;
  Result(string const& rule_, float cost_, Result *left=NULL, Result *right=NULL) : rule(rule_), history(rule_,0,1), cost(cost_) {
    child[0]=left;child[1]=right;
    if (child[0]) {
      cost+=child[0]->cost;
      if (child[1])
        cost+=child[1]->cost;
    }
  }
  friend ostream & operator <<(ostream &o, Result const& v)
  {
    o << "cost=" << v.cost << " tree={{{";
    v.print_tree(o, false);
    o <<"}}} derivtree={{{";
    v.print_tree(o);
    return o << "}}} history={{{" << v.history << "}}}";
  }
  Result(Result *prototype, Result *old_child, Result *new_child, lazy_kbest_index_type which_child) {
    rule=prototype->rule;
    child[0]=prototype->child[0];
    child[1]=prototype->child[1];
    assert(which_child<2);
    assert(child[which_child]==old_child);
    child[which_child]=new_child;
    cost = prototype->cost + - old_child->cost + new_child->cost;
    EIFDBG(LAZYF,7,
           KBESTNESTT;
           KBESTINFOT("NEW RESULT proto=" << *prototype << " old_child=" << *old_child << " new_child=" << *new_child << " childid=" << which_child << " child[0]=" << child[0] << " child[1]=" << child[1]));

    std::ostringstream newhistory, newtree, newderivtree;
    newhistory << prototype->history << ',' << (which_child ? "R" : "L") << '-' << old_child->cost << "+" << new_child->cost;
    //<< '(' << new_child->history << ')';
    history = newhistory.str();
  }
  bool operator < (Result const& other) const {
    return cost > other.cost;
  } // worse < better!
  void print_tree(ostream &o, bool deriv=true) const
  {
    if (deriv)
      o << "[" << rule << "]";
    else {
      o << rule.substr(rule.find("->")+2,1);
    }
    if (child[0]) {
      o << "(";
      child[0]->print_tree(o, deriv);
      if (child[1]) {
        o << " ";
        child[1]->print_tree(o, deriv);
      }
      o << ")";
    }
  }

};

//Result::Factory::derivation_type Result::Factory::NONE=0, Result::Factory::PENDING=(Result*)0x1;

struct ResultPrinter {
  bool operator()(const Result *r, unsigned i) const {
# if USE_DEBUGPRINT
    KBESTNESTT;
    KBESTINFOT("Visiting result #" << i << " = " << r);
    KBESTINFOT("");
# endif
    cout << "RESULT #" << i << "=" << *r << "\n";
# if USE_DEBUGPRINT
    KBESTINFOT("done #:" << i);
# endif
    return true;
  }
};

typedef lazy_forest<Result::Factory> LK;

/*
  qe
  qe -> A(qe qo) # .33 a
  qe -> A(qo qe) # .33 b
  qe -> B(qo) # .34 c
  qo -> A(qo qo) # .25 d
  qo -> A(qe qe) # .25 e
  qo -> B(qe) # .25 f
  qo -> C # .25 g

  .25 -> 1.37
  .34 -> 1.08
  .33 -> 1.10
*/

inline void jonmay_cycle(unsigned N=25, int weightset=0)
{
  using std::log;
  LK::lazy_forest qe,
    qo;
// float ca=1.1, cb=1.1, cc=1.08, cd=1.37, ce=1.1, cf=1.37, cg=1.37;
  /*
    ca=cb=1.1;
    cc=1.08;
    cd=ce=cf=cg=1.37;
  */
  float ca=.502, cb=.491, cc=0.152, cd=.603, ce=.502, cf=.174, cg=0.01;

  if (weightset==2) {
    ca=cb=cc=cd=ce=cf=cg=1;
  }
  if (weightset==1) {
    ca=cb=-log(.33);
    cc=-log(.34);
    cd=ce=cf=cg=-log(.25);
  }

  Result g("qo->C", cg);
  Result c("qe->B(qo)", cc, &g);
  Result a("qe->A(qe qo)", ca, &c, &g);
  Result b("qe->A(qo qe)", cb, &g, &c);
  Result d("qo->A(qo qo)", cd, &g, &g);
  Result e("qo->A(qe qe)", ce, &c, &c);
  Result f("qo->B(qe)", cf, &c);


  qe.add_sorted(&c, &qo);
  qe.add_sorted(&a, &qe, &qo);
  qe.add_sorted(&b, &qo, &qe);
  assert(qe.is_sorted());

  qo.add(&e, &qe);
  qo.add(&g);
  qo.add(&d, &qo, &qo);
  qo.add(&f, &qe);

  assert(!qo.is_sorted());
  qo.sort();
  assert(qo.is_sorted());
# if USE_DEBUGPRINT
  NESTT;
# endif
  //LK::enumerate_kbest(10, &qo, ResultPrinter());
  qe.enumerate_kbest(N, ResultPrinter());
}

inline void simplest_cycle(unsigned N=25)
{
  float cc=0, cb=1, ca=.33, cn=-10;
  Result c("q->C", cc);
  Result b("q->B(q)", cb, &c);
  Result n("q->N(q)", cn, &c);
  Result a("q->A(q q)", ca, &c, &c);
  LK::lazy_forest q, q2;
  q.add(&a, &q, &q);
  q.add(&b, &q);
  q.add(&n, &q);
  q.add(&c);
  assert(!q.is_sorted());
  q.sort();
// assert(q.is_sorted());
# if USE_DEBUGPRINT
  NESTT;
# endif
  q.enumerate_kbest(N, ResultPrinter());
}

inline void simple_cycle(unsigned N=25)
{

  float cc=0;
  Result c("q->C", cc);
  float cd=.01;
  Result d("q2->D(q)", cd, &c);
  float cneg=-10;
  Result eneg("q2->E(q2)", cneg, &d);
  float cb=.1;
  Result b("q->B(q2 q2)", cb, &d, &d);
  float ca=.5;
  Result a("q->A(q q)", ca, &c, &c);
  LK::lazy_forest q, q2;
  q.add(&a, &q, &q);
  q.add(&b, &q2, &q2);
  q.add(&c);
  assert(!q.is_sorted());
  q.sort();
  assert(q.is_sorted());
  q2.add_sorted(&d, &q);
  q2.add_sorted(&eneg, &q2);
// assert(q2.is_sorted());
// q2.sort();
// DBP2(*q2.pq[0].derivation,*q2.pq[1].derivation);
  // assert(!q2.is_sorted());
//  DBP2(eneg, d);
//  NESTT;
  q.enumerate_kbest(N, ResultPrinter());
}


inline void jongraehl_example(unsigned N=25)
{
  float eps=-1000;
  LK::lazy_forest a, b, c, f;
  float cf=-2;Result rf("f->F", cf);
  float cb=-5;Result rb("b->B", cb);
  float cfb=eps;Result rfb("f->I(b)", cfb, &rb);
  float cbf=-cf+eps;Result rbf("b->H(f)", cbf, &rf);
  float cbb=10;Result rbb("b->G(b)", cbb, &rb);
  float ccbf=8;Result rcbf("c->C(b f)", ccbf, &rb, &rf);
  float cabc=6;Result rabc("a->A(b c)", cabc, &rb, &rcbf);
  float caa=-cabc+eps;Result raa("a->Z(a)", caa, &rabc);
// Result ra("a",6), rb("b",5), rc("c",1), rf("d",2), rb2("B",5), rb3("D",10), ra2("A",12);
  f.add_sorted(&rf); // terminal
  f.add_sorted(&rfb);
  b.add_sorted(&rb); // terminal
  b.add_sorted(&rbf, &f);
  b.add_sorted(&rbb, &b);
  c.add_sorted(&rcbf, &b, &f);
  a.add_sorted(&rabc, &b, &c);
  a.add_sorted(&raa, &a);
  assert(a.is_sorted());
  assert(b.is_sorted());
  assert(c.is_sorted());
  assert(f.is_sorted());
  NESTT;
  LK::throw_on_cycle=true;
  f.enumerate_kbest(1, ResultPrinter());
  b.enumerate_kbest(1, ResultPrinter());
  c.enumerate_kbest(1, ResultPrinter());
  a.enumerate_kbest(N, ResultPrinter());
}

inline void all_examples(unsigned N=30)
{
  simplest_cycle(N);
  simple_cycle(N);
  jongraehl_example(N);
  jonmay_cycle(N);
}

}//lazy_forest_kbest_example
#endif

# ifdef TEST
BOOST_AUTO_TEST_CASE(TEST_lazy_kbest) {
  lazy_forest_kbest_example::all_examples();

}
#endif

}//graehl

#ifdef LAZY_FOREST_SAMPLE
int main()
{
  using namespace graehl::lazy_forest_kbest_example;
  unsigned N=30;
  simplest_cycle(N);
// all_examples();
  return 0;
  /*
    if (argc>1) {
    char c=argv[1][0];
    if (c=='1')
    simple_cycle();
    else if (c=='2')
    simplest_cycle();
    else
    jongraehl_example();
    } else
    jonmay_cycle();
  */
}
#endif


}

#endif
