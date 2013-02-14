/** \file

 input: running text on stdin

 output: list of "word line#" for all the one-count words

 (C++11 for unordered_map)

*/

#include <unordered_map>
#include <string>
#include <iostream>

using namespace std;

void usage() {
  cerr<<"input (stdin): space separated words\noutput (stdout): for all the one-count words, lines of 'word line#'\n";
}

unsigned const bufsz=1024*32;
char buf[bufsz];

unsigned const kRepeated=0;

typedef unordered_map<string,unsigned> WordLine; // or kRepeated if it appeared >1

WordLine wordLine;

string line;
unsigned lineno;

void addWord(std::string const& word) {
  pair<WordLine::iterator,bool> iNew=wordLine.insert(pair<string,unsigned>(word,lineno));
  if (!iNew.second)
    iNew.first->second=kRepeated;
}

void read(istream &in)
{
  while (getline(in,line)) {
    ++lineno;
    typedef string::const_iterator Iter;
    for(Iter i=line.begin(),N=line.end(); ; ) {
      while(i!=N && *i==' ') ++i;
      if (i==N) break;
      Iter startWord=i;
      ++i;
      while(i!=N && *i!=' ') ++i;
      addWord(string(startWord,i));
    }
  }
}

void write(ostream &out) {
  for (WordLine::const_iterator i=wordLine.begin(),e=wordLine.end();i!=e;++i)
    if (i->second!=kRepeated)
      out<<i->first<<' '<<i->second<<'\n';
}


int main(int argc,char *argv[]) {
  if (argc==2 && argv[1][0]=='-' && (argv[1][1]=='h' || argv[1][1]=='-' && argv[1][2]=='h')) {
    usage();
    return 1;
  }

  ios_base::sync_with_stdio(false);
  cin.tie(0);
  cin.rdbuf()->pubsetbuf(buf,bufsz);

  wordLine.reserve(1000*1000);

  read(cin);
  write(cout);
  return 0;
}
