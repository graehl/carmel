#include <graehl/shared/config.h>
#include <string>
#include <map>
#include <graehl/shared/myassert.h>
#include <graehl/carmel/src/fst.h>
#include <iterator>
#include <sstream>
#include <graehl/shared/io.hpp>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/input_error.hpp>
#include <graehl/shared/assoc_container.hpp>

namespace graehl {

static const int DEFAULTSTRBUFSIZE=4096;

#define REQUIRE(x)  do { if (!(x)) { goto INVALID; } } while(0)
#define GETC do { REQUIRE(istr >> c); } while(0)
#define PEEKC  do { REQUIRE(istr>>c); istr.unget(); } while(0)
#define BOOLBRIEF bool brief = (os.iword(arcformat_index) == BRIEF)
#define OUTARCWEIGHT(os,a)              do { int pGroup = (a)->groupId; \
        if ( !brief || pGroup >= 0 || (a)->weight != 1.0 )              \
            os << " " << (a)->weight;                                   \
        if ( pGroup >= 0 ) {                                            \
            os << '!';                                                  \
            if ( pGroup > 0)                                            \
                os << pGroup;                                           \
        } } while(0)


static inline unsigned int pow2(int exp)
{
    return 1 << exp;
}


#define DOS_CR_CHAR '\r'
static char *getString(std::istream &in, char *buf,unsigned STRBUFSIZE=DEFAULTSTRBUFSIZE)
{
#define CHECKBUFOVERFLOW do {                   \
        if (buf >= bufend)                      \
            goto bufoverflow;                   \
    } while(0)
    
    int l;
    char *s,*bufend=buf+STRBUFSIZE-2;
    if ( !(in >> *buf) ) return 0;
    switch ( *buf ) {
    case '"':
        l = 0;                      // 1 if backslash was last character
        for ( s = buf+1 ; s < bufend ; ++s ) {
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
        CHECKBUFOVERFLOW;
        break;
    case '*':                     // all strings delimited by * are special symbols
        for ( s = buf+1 ; s < bufend ; ++s ) {
            if ( !in.get(*s) ) return 0;
            if ( *s == '*' )
                break;
            else
                *s = tolower(*s);
        }
        *++s = '\0';
        CHECKBUFOVERFLOW;
        break;
    case '(':
    case ')':
        return 0;
        break;
    default:
        while ( in.get(*++buf)) {
            CHECKBUFOVERFLOW;
            if (*buf == '\n' || *buf == '\t' || *buf == ' ')
                break;
            if (*buf == '!' || *buf == ')' ) {
                in.unget();
                break;
            }
        }        
        *buf = '\0';
        if (buf[-1] == DOS_CR_CHAR)
            *--buf = '\0';
        break;
    }
    return buf;
bufoverflow:
    *bufend='\0';
    std::cerr << "Symbol too long (over "<<bufend-buf<<" characters): " << buf;
    return 0;
#undef CHECKBUFOVERFLOW
}

WFST::WFST(const char *buf)
{
    named_states=0;
    initAlphabet();
    istringstream line(buf);
    char symbol[DEFAULTSTRBUFSIZE];
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
        push_back(states);
        states[final].addArc(FSTArc(symbolInNumber, symbolOutNumber, final + 1, 1.0));
        ++final;
    }
    push_back(states);                    // final state
}

struct ltstr // Yaser 8-3-200
{
    bool operator()(const char* s1, const char* s2) const
    {
        return strcmp(s1, s2) < 0;
    }
};

bool isNonNegInt(const char * p){
    //FIXME: was p+1 and not p++ intended??? would be ignoring 1st char ...
    const char *q = p++ + strlen(p);
    while((p < q ) && isdigit(*p)){p++;}
    return(p==q);
}


WFST::WFST(const char *buf, int &length,bool permuteNumbers)
// Generate a permutation lattice for a given string
{
    named_states=0;
    initAlphabet();
    length = 0 ;
    istringstream line(buf);
    char symbol[DEFAULTSTRBUFSIZE];
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
            if (isNonNegInt(symbol)){ /*it is alphanumeric symbol*/
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
        push_back(states);                  /* final state*/
        final = pow2((int)symbols.size())-1;
        for (unsigned int k=0; k < final; k++){
            push_back(states);
            vector<bool> taken(maxSymbolNumber+1);
            for (int i = 0 ; i <= maxSymbolNumber ;++i)
                taken[i] = false ;
            for (int l=0; l < int(symbols.size()); l++){
                int temp = pow2(l);
                if (((int(k / temp) % 2) == 0) && (!taken[unsigned(symbols[l])])){
                    states[k].addArc(FSTArc(symbols[l], symbols[l], k+temp, 1.0));
                    taken[symbols[l]] = true ;
                }
            }
        }
    }
    else{
        final = pow2((int)strSymbols.size())-1 ;
        for (unsigned int k=0; k <= final; k++){
            push_back(states);
        }
        int temp_final = final ;
        //    vector<bool> visited(final,false);
        vector<bool> visited(final);
        for (unsigned int k=0; k < final; k++)
            visited[k] = false ;
        visited[0] = true ;
        for (unsigned int k=0; k < final; k++){
            if (visited[k]){
                map<const char*, bool, ltstr> taken ;
                for (unsigned int i = 0 ; i < strSymbols.size() ;++i)
                    taken[strSymbols[i].c_str()] = false ;
                for (int l=0; l < int(strSymbols.size()); l++){
                    int temp = pow2(l);
                    if (((int(k / temp) % 2) == 0) && (!taken[strSymbols[l].c_str()])){
                        if (isNonNegInt(strSymbols[l].c_str())){
                            unsigned int from_state,to_state ;
                            from_state = k ;
                            for(unsigned int i =1 ; i < strSymbols[l].length()-1 ; i++){
                                string s("\"\"\"");
                                s[1] = strSymbols[l][i];
                                if (NULL == in->find(const_cast<char *>(s.c_str())) || NULL == out->find(const_cast<char *>(s.c_str()))){
                                    std::cerr << "problem! didn't find "<< s << '\n';
                                }
                                else {
                                    while (from_state >= states.size()) // (from_state >= numStates()
                                        push_back(states);
                                    if (i == strSymbols[l].length()-2)
                                        to_state = k+temp ;
                                    else
                                        to_state =  ++temp_final ;
                                    if (from_state >= states.size())
                                        states.resize(from_state);
                                    //              std::cerr << "adding arc (from:" << from_state << ", to:"<<to_state <<", in/out:"<<s <<"("<<in->indexOf(const_cast<char *>(s.c_str()))<<"))\n";
                                    states[from_state].addArc(FSTArc(in->indexOf(const_cast<char *>(s.c_str())),in->indexOf(const_cast<char *>(s.c_str())) ,to_state, 1.0));
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
                                states[k].addArc(FSTArc(in->indexOf(const_cast<char *>(strSymbols[l].c_str())),out->indexOf(const_cast<char *>(strSymbols[l].c_str())) ,k+temp, 1.0));
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


int WFST::getStateIndex(const char *buf) {
    char *scanend;
    unsigned int st;
    if (!named_states) {
        st=strtol(buf,&scanend,10); // base 10, potential buffer overflow?? not really, read only
        if (*buf && *scanend != '\0') {
            Config::warn() << "Since intial state was a number, expected an integer state index, but got: " << buf << std::endl << "\t(-K command line option should not be used if states are named)" << std::endl;
            return -1;
        } else {
            resize_up_for_index(states,st);
            return st;
        }
    } else {
        st = stateNames.indexOf((char *)buf);
        if ( st >= states.size() ) {
            push_back(states);
            Assert(st + 1 == states.size());
        }
        return st;
    }
}

static const char COMMENT_CHAR='%';

//FIXME: need to destroy old data or switch this to a constructor
int WFST::readLegible(istream &istr,bool alwaysNamed)
{
    StringKey finalName;
    try {        
        named_states=1;
        int stateNumber, destState, inL, outL;
        Weight weight;
        char c, d;
        char buf[DEFAULTSTRBUFSIZE],buf2[DEFAULTSTRBUFSIZE];
//        string buf,buf2; // FIXME: rewrite getString to use growing buffer?
        skip_comment(istr,COMMENT_CHAR);
        REQUIRE(getString(istr, buf));
        finalName = buf;

        if (!alwaysNamed) {
            named_states=0;
            for (const char *p=finalName.c_str();*p;++p)
                if (!isdigit(*p)) {
                    named_states=1;
                    break;
                }
        }

        if (named_states)
            finalName.clone();
        else
            final=getStateIndex(buf);
        Assert( *in->find(EPSILON_SYMBOL)==0 && *out->find(EPSILON_SYMBOL)==0 );
        while ( istr >> c ) {
            // begin line:
            skip_comment(istr,COMMENT_CHAR);
            REQUIRE(c == '(');
            // start state:
            REQUIRE(getString(istr, buf));

            stateNumber=getStateIndex(buf);
            if (stateNumber == -1)
                goto INVALID;
            
            // PRE: read: '(' source
            // expecting: {[destparen(] dest ... [destparen)]}* ')'
            for (;;) {
                GETC;
                bool destparen=(c=='(');
                if (!destparen)
                    istr.unget();
                if( c == ')' )
                    break;
      
                // dest state:
                REQUIRE(getString(istr, buf));
                destState=getStateIndex(buf);
                if (destState == -1)
                    goto INVALID;

                // (iow!g)*
                for(;;) {
                    GETC;
                    bool iowparen=(c=='(');
                    if (!iowparen)
                        istr.unget();
                    else
                        PEEKC;
                    //PRE: read: '(' source [destparen(] dest [iowparen(]
                    //expecting: if iowparen, repetitions of: [[input [output]] weight] [![group]] ')'
                    //             reps separated by '('/REPEAT OR ')'/END 
                    //   else, single repetition
#define ENDIOW (c==')'||c=='!')                    
                    if (ENDIOW) { // ...)
                        inL=outL=WFST::epsilon_index;
                        weight=1.0;
                    } else {
                        char *e;
#define GETBUF(buf) REQUIRE(e=getString(istr,buf))
                        GETBUF(buf);
                        PEEKC;
                        if (ENDIOW) {  // ... weight) or ... symbol)
                            if (weight.setString(buf)) { // ... weight)
                                inL=outL=WFST::epsilon_index;
                            } else { // ... symbol)
                                inL = in->indexOf(buf);
                                outL = out->indexOf(buf);
                                weight=1.0;
                            }              
                        } else {
                            inL = in->indexOf(buf);
                            GETBUF(buf2);
                            PEEKC;
                            if (ENDIOW) {  // ... iosymbol weight) or ... isymbol osymbol)
                                if (weight.setString(buf2)) { // ... iosymbol weight)
                                    outL = out->indexOf(buf);
                                } else { // ... iosymbol osymbol) 
                                    outL = out->indexOf(buf2);
                                    weight=1.0;
                                }        
                            } else { // ... isymbol osymbol weight)
                                outL = out->indexOf(buf2);
                                REQUIRE(istr >> weight);
                                PEEKC;
                                REQUIRE(ENDIOW);
                            } 
                        }
                    }
                    
                    // POST: read iow sequence
                    // expecting: [ '!' [groupid]]
#undef GETBUF
//                    DBP5(stateNumber,destState,inL,outL,weight);                    
                    states[stateNumber].addArc(FSTArc(inL, outL, destState, weight)); //TODO: use back_insert_iterator so arc list doesn't get reversed? or print out in reverse order?
                    GETC;
                    if ( c == '!' ) { // lock weight
                        FSTArc *lastAdded = &states[stateNumber].arcs.top();
                        PEEKC;
                        if( isdigit(c) ) {
                            int group;
                            REQUIRE(istr >> group);
                            //      tieGroup.insert(IntKey(int(lastAdded)), group);
                            lastAdded->setGroup(group);
                        } else {
                            lastAdded->setLocked();
                        }                        
                    } else
                        istr.unget();
                    // POST: finished reading: iow!g)
                    if (!iowparen)
                        break;
                    REQUIRE(istr >> c && c== ')');
                    PEEKC;
                    if (c==')')
                        break;                        
                }
                if (!destparen)
                    break;       
                REQUIRE(istr >> c && c== ')');
            }
            REQUIRE(istr >> c && c== ')');
            // POST: finished with (dest (iow!g)*)* (done with "line")
        }        
        // POST: no more input
        if ( !named_states) {
            if (!(final < size())) goto INVALID; // whoops, this can never happen because of getStateIndex creating the (empty) state

            return 1;
        }
        {
            unsigned const*uip = stateNames.find(finalName);
            if ( uip  ) {
                final = *uip;
                finalName.kill();
                return 1;
            } else {
                cout << "\nFinal state named " << finalName << " not found.\n";
                goto INVALID;
            }
        }
    } catch (std::exception &e) {
        goto INVALID;        
    }
INVALID:
    show_error_context(istr,cerr);
    if (named_states)
        finalName.kill();
    invalidate();
    return 0;
}
/*          
            buf[0]='*';buf[1]='e';buf[2]='*';buf[3]='\0';
            GETC;  // skip whitespace
            istr.unget();
            if (!(isdigit(c) || c == '.' || c == '-' || c == ')'))
            REQUIRE(getString(istr, buf));
            GETC;  // skip whitespace
            istr.unget();
            if (!(isdigit(c) || c == '.' || c == '-' || c == ')'))
            REQUIRE(getString(istr, buf));
            outL = out->indexOf(buf);
            GETC; // skip ws
            istr.unget();
            weight.setZero();
            if (isdigit(c) || c == '.' || c == '-' ) {
            REQUIRE(istr >> weight);
            if ( istr.fail() ) {
            cout << "Invalid weight: " << weight <<"\n";
            return 0;
            }
            } else
            weight = 1.0;
            //        if ( weight > 0.0 ) {
            */
//} else if ( weight != 0.0 ) {
//          cout << "Invalid weight (must be nonnegative): " << weight <<"\n";
//          return 0;
//}


int WFST::readLegible(const string& str,bool alwaysNamed)
{
    istringstream istr(str);
    return(readLegible(istr,alwaysNamed));
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
    const char * const prelude="digraph G {";
    //size=\"7.5,10\",
    const char * const format="graph[page=\"8.5,11\",center=1,orientation=landscape]";
    const char * const coda = ";\n}\n";
    const char * const final_border = "peripheries=2";
    const char * const state_shape = "node [shape=circle]"; // "shape=ellipse"
    const char * const arrow = " -> ";
    const char * const open = " [";
    const char close = ']';
    const char * const label = "label=";

    os << prelude << endl;
    os << format << newl << invis_start << newl << state_shape;

    // make sure final state gets double circle
    os << newl;
    writeQuoted(os,stateName(final));
    os << open << final_border << close;

    // arc from invisible start to real start
    os << newl << invis_start_name << arrow;
    writeQuoted(os,stateName(0));

    for (int s = 0 ; s < numStates() ; s++) {
        for (List<FSTArc>::const_iterator a=states[s].arcs.const_begin(),end = states[s].arcs.const_end() ; a !=end ; ++a ) {
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

void WFST::writeArc(ostream &os, const FSTArc &a,bool GREEK_EPSILON) {
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
    Assert( *in->find(EPSILON_SYMBOL)==0 && *out->find(EPSILON_SYMBOL)==0 );
    os << stateName(final);
    for (i = 0 ; i < numStates() ; i++) {
        if (!onearc)
            os << "\n(" << stateName(i);
        for (List<FSTArc>::const_iterator a=states[i].arcs.const_begin(),end = states[i].arcs.const_end() ; a !=end ; ++a ) {

            if ( a->weight.isPositive() ) {
                if (onearc)
                    os << "\n(" << stateName(i);
                destState = stateName(a->dest);
                os << " (" << destState;
                if ( !brief || a->in || a->out ) { // omit *e* *e* labels
                    inLet = (*in)[a->in].c_str();
                    outLet = (*out)[a->out].c_str();
                    os << " " << inLet;
                    if ( !brief || strcmp(inLet, outLet) )
                        os << " " << outLet;
                }
                //      int *pGroup;
                //      if ( (pGroup = tieGroup.find_second(IntKey(int(&(*a))))) ) {
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
    Alphabet<StringKey,StringPool> *alph;
    if ( output )
        alph = out;
    else
        alph = in;
    ostr << *alph;
}

void WFST::symbolList(List<int> *ret,const char *buf, int output,int lineno) const
{
//  List<int> *ret = NEW List<int>;
    
    //LIST_BACK_INSERTER<List<int> > cursor(*ret);
//  insert_iterator<List<int> > cursor(*ret,ret->erase_begin());
    List<int>::back_insert_iterator cursor(*ret);
    //  ListIter<int> ins(*ret);
    istringstream line(buf);
    char symbol[DEFAULTSTRBUFSIZE];
    Alphabet<StringKey,StringPool> *alph;
    if ( output )
        alph = out;
    else
        alph = in;
    while ( line ) {
        if ( !getString(line, symbol) )
            break;
#if WFSTIO_ERROR_SEQUENCE_NOT_IN_ALPHABET
        unsigned const*pI = alph->find(symbol);
        if ( !pI) {
//      delete ret;
//      return NULL;

            std::ostringstream o;
            o << "Input sequence has symbol not in alphabet: "<<symbol;
            if (lineno >= 0)
                o << " on line " << lineno;
            throw std::runtime_error(o.str());
        } else
            //      ins.insert(*pI);
            //      ret->insert(ret->begin(),*pI);
            *cursor++ = *pI;
#else
        *cursor++ = alph->index_of(symbol);
#endif 
    }
//  return ret;
}

}

#undef REQUIRE
#undef PEEKC
#undef OUTARCWEIGHT
#undef BOOLBRIEF
#undef DOS_CR_CHAR
