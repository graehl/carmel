#include <string>
#include <map>
#include "assert.h"
#include "fst.h"
#include <iterator>

#define DO(x)  { if (!(x)) return 0; }

#define BOOLBRIEF bool brief = (os.iword(arcformat_index) == BRIEF)
#define OUTARCWEIGHT(os,a) 		do { int pGroup = (a)->groupId; \
        if ( !brief || pGroup >= 0 || (a)->weight != 1.0 ) \
          os << " " << (a)->weight; \
        if ( pGroup >= 0 ) { \
          os << '!'; \
          if ( pGroup > 0) \
            os << pGroup; \
		} } while(0)


static int pow2(int exp)
{
        return 1 << exp;
}

static int getString(istream &in, char *buf)
{
  int l;
  char *s;
  if ( !(in >> buf[0]) ) return 0;
  switch ( buf[0] ) {
  case '"':
    l = 0;                      // 1 if backslash was last character
    for ( s = buf+1 ; s < buf+4094 ; ++s ) {
      if ( !in.get(*s) ) return 0;
      if ( *s == '"' )
        if ( !l )
          break;
      if ( *s == '\\' )
        l = l ^ 1;              // toggle escape state
      else
        l = 0;
    }
    *++s = '\0';
    if ( s >= buf+4094 ) {
      std::cerr << "Symbol too long (over 4000 characters): " << buf;
      return 0;
    }
    break;
  case '*':                     // all strings delimited by * are special symbols
    for ( s = buf+1 ; s < buf+4094 ; ++s ) {
      if ( !in.get(*s) ) return 0;
      if ( *s == '*' )
        break;
      else
        *s = tolower(*s);
    }
    *++s = '\0';
    if ( s >= buf+4094 ) {
      std::cerr << "Symbol too long (over 4000 characters): " << buf;
      return 0;
    }
    break;
  case '(':
  case ')':
    return 0;
    break;
  default:
    while ( in.get(*++buf) && *buf != '\n' && *buf != '\t' && *buf != ' ' && *buf != ')' )
      ;
    if ( *buf == ')' )
      in.putback( ')' );
    *buf = 0;
    break;
  }
  return 1;
}

WFST::WFST(const char *buf) : ownerInOut(1), in(new Alphabet("*e*")),  out(new Alphabet("*e*")), trn(NULL)
{
  istringstream line(buf);
  char symbol[4096];
  int symbolInNumber, symbolOutNumber;
  final = 0;
  while ( line ) {
    if ( !getString(line, symbol) )
      break;
    if ( isdigit(symbol[0]) ) {
      invalidate();
      return;
    }
    symbolInNumber = in->indexOf(symbol);
    symbolOutNumber = out->indexOf(symbol);
    Assert (symbolInNumber == symbolOutNumber);
    states.pushBack();
    states[final].addArc(Arc(symbolInNumber, symbolOutNumber, final + 1, 1.0));
    ++final;
  }
  states.pushBack();                    // final state
}

struct ltstr // Yaser 8-3-200
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

bool isNumber(const char * p){
  const char *q = p++ + strlen(p) ;
  while((p < q ) && isdigit(*p)){p++;}
  return(p==q);
}


WFST::WFST(const char *buf, int &length,bool permuteNumbers) : ownerInOut(1), in(new Alphabet("*e*")),  out(new Alphabet("*e*")), trn(NULL) // Yaser 7-25-2000
// Generate a permutation lattice for a given string
{
  length = 0 ;
  istringstream line(buf);
  char symbol[4096];
  vector<int> symbols;
  vector<string> strSymbols;
  string currSym("") ;
  int symbolInNumber, symbolOutNumber,maxSymbolNumber=0 ;
  final = 0;
  while ( line ) {
    if ( !getString(line, symbol) ){
      if(!permuteNumbers && currSym != ""){
        strSymbols.push_back("\"" + currSym + "\"");
      }
      break;
    }
    length++ ;
    if ( isdigit(symbol[0]) ) {
      invalidate();
      return;
    }
    if (!permuteNumbers){
      if (isNumber(symbol)){ /*it is alphanumeric symbol*/
        string temp(symbol+1);
        currSym += temp.substr(0,temp.length()-1)  ;
      }
      else{
        if(currSym != ""){
          strSymbols.push_back("\"" + currSym + "\"");
        }
        strSymbols.push_back(symbol);
        currSym="";
      }
    }
    symbolInNumber = in->indexOf(symbol);
    symbolOutNumber = out->indexOf(symbol);
    Assert (symbolInNumber == symbolOutNumber);
    symbols.push_back(symbolInNumber);
    if (maxSymbolNumber < symbolInNumber)
      maxSymbolNumber = symbolInNumber ;
  }
  if (permuteNumbers){
    states.pushBack();                  /* final state*/
    final = pow2((int)symbols.size())-1;
    for (int k=0; k < final; k++){
      states.pushBack();
      vector<bool> taken(maxSymbolNumber+1);
      for (int i = 0 ; i <= maxSymbolNumber ;++i)
        taken[i] = false ;
      for (int l=0; l < int(symbols.size()); l++){
        int temp = pow2(l);
        if (((int(k / temp) % 2) == 0) && (!taken[unsigned(symbols[l])])){
          states[k].addArc(Arc(symbols[l], symbols[l], k+temp, 1.0));
          taken[symbols[l]] = true ;
        }
      }
    }
  }
  else{
    final = pow2((int)strSymbols.size())-1 ;
    for (int k=0; k <= final; k++){
      states.pushBack();
    }
    int temp_final = final ;
    //    vector<bool> visited(final,false);
    vector<bool> visited(final);
    for (int k=0; k < final; k++)
      visited[k] = false ;
    visited[0] = true ;
    for (int k=0; k < final; k++){
      if (visited[k]){
        map<const char*, bool, ltstr> taken ;
        for (unsigned int i = 0 ; i < strSymbols.size() ;++i)
          taken[strSymbols[i].c_str()] = false ;
        for (int l=0; l < int(strSymbols.size()); l++){
          int temp = pow2(l);
          if (((int(k / temp) % 2) == 0) && (!taken[strSymbols[l].c_str()])){
            if (isNumber(strSymbols[l].c_str())){
              int from_state,to_state ;
              from_state = k ;
              for(unsigned int i =1 ; i < strSymbols[l].length()-1 ; i++){
                string s("\"\"\"");
                s[1] = strSymbols[l][i];
                if (NULL == in->find(const_cast<char *>(s.c_str())) || NULL == out->find(const_cast<char *>(s.c_str()))){
                  std::cerr << "problem! didn't find "<< s << '\n';
                }
                else {
                  while (from_state >= states.count()) // (from_state >= numStates()
                    states.pushBack();
                  if (i == strSymbols[l].length()-2)
                    to_state = k+temp ;
                  else
                    to_state =  ++temp_final ;
                  if (from_state >= states.count())
                    states.resize(from_state);
                  //              std::cerr << "adding arc (from:" << from_state << ", to:"<<to_state <<", in/out:"<<s <<"("<<in->indexOf(const_cast<char *>(s.c_str()))<<"))\n";
                  states[from_state].addArc(Arc(in->indexOf(const_cast<char *>(s.c_str())),in->indexOf(const_cast<char *>(s.c_str())) ,to_state, 1.0));
                  from_state=to_state ;
                }
              }
            }
            else{
              if (NULL == in->find(const_cast<char *>(strSymbols[l].c_str())) || NULL == out->find(const_cast<char *>(strSymbols[l].c_str()))){
                std::cerr << "Error in constructing a permutation lattice!! didn't find symbol in the alphabet "<< strSymbols[l] << '\n';
              }
              else {
                //              std::cerr << "adding arc (from:" << k << ", to:"<<k+temp <<", in/out:"<<strSymbols[l].c_str() << '\n';
                states[k].addArc(Arc(in->indexOf(const_cast<char *>(strSymbols[l].c_str())),out->indexOf(const_cast<char *>(strSymbols[l].c_str())) ,k+temp, 1.0));
              }
            }
            /*symbols[l], symbols[l], k+temp, 1.0));*/
            taken[strSymbols[l].c_str()] = true ;
            visited[k+temp] = true ;
          }
        }
      }
    }
  }
  reduce();
}

static const char COMMENT_CHAR='#';

// need to destroy old data or switch this to a constructor
int WFST::readLegible(istream &istr)
{
  int stateNumber, destState, inL, outL;
  Weight weight;
  char c, d, buf[4096];
  StringKey finalName;
  DO(getString(istr, buf));
  finalName = buf;
  finalName.clone();
  Assert( in->find("*e*") && out->find("*e*") && !in->indexOf("*e*") && !out->indexOf("*e*") );
  for ( ; ; ) {
    if ( !(istr >> c) )
      break;
	// begin line:
	if (c == COMMENT_CHAR) {
		for(;;) {
			if (!istr.get(c) )
				break;
			if (c == '\n')
				break;
		}
		continue;
	}
    DO(c == '(');
	// start state:
    DO(getString(istr, buf));
    stateNumber = stateNames.indexOf(buf);
    if ( stateNumber >= states.count() ) {
      states.pushBack();
      Assert(stateNumber + 1 == states.count());
    }
	// arcs:
    for ( ; ; ) {
      DO(istr >> c);
      if( c == ')' )
        break;
      DO(c == '(');
	  // dest state:
      DO(getString(istr, buf));
      destState = stateNames.indexOf(buf);
      if ( destState >= states.count() ) {
        states.pushBack();
        Assert(destState + 1 == states.count());
      }
      DO(istr >> d);
      if ( d != '(' )
        istr.putback(d);
      for ( ; ; ) {
                 buf[0]='*';buf[1]='e';buf[2]='*';buf[3]='\0';
                 DO(istr >> c);  // skip whitespace
                 istr.putback(c);
                 if (!(isdigit(c) || c == '.' || c == '-' || c == ')'))
                        DO(getString(istr, buf));
        inL = in->indexOf(buf);
        DO(istr >> c);  // skip whitespace
        istr.putback(c);
        if (!(isdigit(c) || c == '.' || c == '-' || c == ')'))
          DO(getString(istr, buf));
        outL = out->indexOf(buf);
        DO(istr >> c); // skip ws
        istr.putback(c);
		weight.setZero();
        if (isdigit(c) || c == '.' || c == '-' ) {
          DO(istr >> weight);
		  if ( istr.fail() ) {
			cout << "Invalid weight: " << weight <<"\n";
			return 0;
		  }
        } else
          weight = 1.0;
//        if ( weight > 0.0 ) {
        states[stateNumber].addArc(Arc(inL, outL, destState, weight));
        //} else if ( weight != 0.0 ) {
//          cout << "Invalid weight (must be nonnegative): " << weight <<"\n";
//          return 0;
        //}
        DO(istr >> c);
        Arc *lastAdded = &states[stateNumber].arcs.top();
        if ( c == '!' ) { // lock weight
          DO(istr >> c); // skip ws
          istr.putback(c);
          int group = 0;
          if( isdigit(c) )
            DO(istr >> group);
          //      tieGroup.add(IntKey(int(lastAdded)), group);
          lastAdded->groupId = group;
        } else
          istr.putback(c);
        if ( d != '(' ) {
          DO(istr >> c);
          DO(c == ')');
          break;
        }
        DO(istr >> c);
        DO(c == ')');
        DO(istr >> c);
        if ( c == ')' )
          break;
        DO(c == '(');
      }
    }
  }
  int *uip;
  if ( (uip = stateNames.find(finalName)) ) {
    final = *uip;
    return 1;
  } else {
    cout << "Final state named " << finalName << " not found.\n";
    invalidate();
    return 0;
  }
}

static ostream & writeQuoted(ostream &os,const char *s) {
	os << '"';
	for (;*s;++s) {
		if (*s == '\\')
			os << '\\' << '\\';
		else {
			if (*s == '"')
				os << '\\';
			os << *s;
		}
	}
	os << '"';
	return os;
}

/*
Uppercase Epsilon is:  &#917;
Lowercase epsilon is:  &#949;
*/
void WFST::writeGraphViz(ostream &os)
{
	if ( !valid() ) return;
	const char *newl = ";\n\t";
	const char * const invis_start="invis_start [shape=plaintext,label=\"\"]";
	const char * const invis_start_name="invis_start";
	const char * const prelude="digraph G {\n";
	const char * const coda = ";\n}\n";
	const char * const final_border = "peripheries=2";
	const char * const state_shape = "node [shape=circle]"; // "shape=ellipse"
	const char * const arrow = " -> ";
	const char * const open = " [";
	const char close = ']';
	const char * const label = "label=";

	os << prelude;
	os << invis_start;
	os << newl << state_shape;

	// make sure final state gets double circle
	os << newl;
	writeQuoted(os,stateName(final));
	os << open << final_border << close;

	// arc from invisible start to real start
	os << newl << invis_start_name << arrow;
	writeQuoted(os,stateName(0));

	for (int s = 0 ; s < numStates() ; s++) {
	    for (List<Arc>::const_iterator a=states[s].arcs.const_begin(),end = states[s].arcs.const_end() ; a !=end ; ++a ) {
			os << newl;
			writeQuoted(os,stateName(s));
			os << arrow;
			writeQuoted(os,stateName(a->dest));
			os << open << label;
			ostringstream arclabel;
			writeArc(arclabel,*a);
			writeQuoted(os,arclabel.str().c_str());
			os << close;
		}
	}

	os << coda;
}

//#define GREEK_EPSILON 1

void WFST::writeArc(ostream &os, const Arc &a,bool GREEK_EPSILON) {
	static const char * const epsilon = "&#949;";
	os << (!GREEK_EPSILON || a.in ? inLetter(a.in) : epsilon) << " : " << (!GREEK_EPSILON || a.out ? outLetter(a.out) : epsilon);
			BOOLBRIEF;
			OUTARCWEIGHT(os,&a);
}

void WFST::writeLegible(ostream &os)
{
  BOOLBRIEF;
  bool onearc = (os.iword(perline_index) == ARC);
  int i;
  const char *inLet, *outLet, *destState;

  if ( !valid() ) return;
  Assert( in->find("*e*") && out->find("*e*") && !in->indexOf("*e*") && !out->indexOf("*e*") );
  os << stateNames[final];
  for (i = 0 ; i < numStates() ; i++) {
    if (!onearc)
		os << "\n(" << stateNames[i];
    for (List<Arc>::const_iterator a=states[i].arcs.const_begin(),end = states[i].arcs.const_end() ; a !=end ; ++a ) {
      if (onearc)
		os << "\n(" << stateNames[i];

     if ( a->weight.isPositive() ) {
        destState = stateNames[a->dest];
        os << " (" << destState;
        if ( !brief || a->in || a->out ) { // omit *e* *e* labels
                inLet = (*in)[a->in];
                outLet = (*out)[a->out];
                os << " " << inLet;
                if ( !brief || strcmp(inLet, outLet) )
                        os << " " << outLet;
        }
        //      int *pGroup;        
        //      if ( (pGroup = tieGroup.find(IntKey(int(&(*a))))) ) {
		OUTARCWEIGHT(os,a);
        os << ")";
		if (onearc)
		  os << ")";
     }
    }
	if (!onearc)
      os << ")";
  }
  os << "\n";
}

void WFST::listAlphabet(ostream &ostr, int output)
{
  Alphabet *alph;
  if ( output )
    alph = out;
  else
    alph = in;
  ostr << *alph;
}

ostream & operator << (ostream &out, Alphabet &alph)
{
  for ( int i = 0 ; i < alph.names.count() ; ++i )
    out << alph.names[i] << '\n';
  return out;
}

//XXX don't allocate then return list, take pointer to list
List<int> *WFST::symbolList(const char *buf, int output) const
{
  List<int> *ret = new List<int>;
  //LIST_BACK_INSERTER<List<int> > cursor(*ret);
  insert_iterator<List<int> > cursor(*ret,ret->erase_begin());
  //  ListIter<int> ins(*ret);
  istringstream line(buf);
  char symbol[4096];
  Alphabet *alph;
  if ( output )
    alph = out;
  else
    alph = in;
  while ( line ) {
    if ( !getString(line, symbol) )
      break;
    int *pI = alph->find(symbol);
    if ( !pI) {
      delete ret;
      return NULL;
    } else
      //      ins.insert(*pI);
      //      ret->insert(ret->begin(),*pI);
      *cursor++ = *pI;
  }
  return ret;
}
