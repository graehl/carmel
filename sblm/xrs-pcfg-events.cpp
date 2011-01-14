/*
  input: xrs rules
  output: (for mapreduce had-rules feature addition, scored w/ external PCFG). id\tlhs r0 r1\n
 */
# include <xrsparse/xrs.hpp>
# include <string>
# include <iostream>
# include <sstream>
# include <stdexcept>

using namespace std;

void visit_pcfg(lhs_pos i,long id) {
  if (!i.can_descend())
    return;  // since p(0 children|lex)=1, we don't emit that event; only show events with nonempty rhs.
    cout << id<<'\t'<<i->label;
    i.descend();
    lhs_pos r=i; // to recurse after we print event.
    for(;;) { // print rhs
      cout<<' ';
      if (i->is_terminal())
        cout<<'"'<<i->label<<'"'; // this is the convention that distinguishes NTs from terminals in rhs. in lowercased systems, it's only punctuation NT/term that would collide.
      else
        cout<<i->label;
      if (!i.can_advance()) break;
      i.advance();
    }
    cout<<"\n";
    for(;;) { // recurse
      visit_pcfg(r,id);
      if (!r.can_advance()) break;
      r.advance();
    }
}


int main(int argc, char** argv)
{
    ios::sync_with_stdio(false);

//    cout << print_rule_data_features(false);
    string line;
    unsigned n=0,good=0,bad=0;
    while (getline(cin,line)) {
      ++n;
        rule_data rd;
        try {
            rd = parse_xrs(line);
            ++good;
            visit_pcfg(rd.lhs_root(),rd.id);
//        cout << rd << '\n';
        } catch(exception const& e) {
          if (!line.empty() && line[0]!='%' && line[0]!='#') {
            cerr << "Bad rule on line "<<n<<": "<<e.what()<<" ("<<line<<")\n";
            ++bad;
          }
        }
    }
    cerr << "DONE.\n"<<n<<" lines, "<<good<<" rules, "<<bad<<" malformed lines.\n";
    return 0;
}
