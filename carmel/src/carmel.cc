//#define MARCU

// -o = learning rate exponent growth ratio (default 1.0)
// -K = don't assume state names are indexes if the final state is an integer
// -q = quiet (default logs computation progress)
// -w w = prune paths ratio (1 = keep only best path, 10 = keep paths up to 10 times worse)
// -z n = keep at most n states
// -U = treat pre-training weights as prior counts
// -Y write graphviz
// -1 = randomly scale weights (of unlocked arcs) after composition uniformly by (0..1]

#include "config.h"
#include <iostream>
#include <fstream>
#include <cctype>
#include <string>
#include <ctime>
#include "fst.h"
#include "assert.h"

#ifdef MARCU
#include "models.h"
char *MarcuArgs[]={
  "marcu-carmel",
  "-IEsriqk",
  "1"
};
#endif

static void setOutputFormat(bool *flags,ostream *fstout) {
  if ( flags['B'] )
    //                      *fstout << Weight::out_log10;
    Weight::out_log10(*fstout);
  else
    //*fstout << Weight::out_ln;
  Weight::out_ln(*fstout);
  if ( flags['Z'] )
    //                      *fstout << Weight::out_always_log;
    Weight::out_always_log(*fstout);
  else
    //*fstout << Weight::out_variable;
  Weight::out_variable(*fstout);
  if ( flags['D'] )
    //                      *fstout << Weight::out_always_real;
    Weight::out_always_real(*fstout);
  if ( flags['J'] )
    //*fstout << WFST::out_arc_full;
  WFST::out_arc_full(*fstout);
  else
    //*fstout << WFST::out_arc_brief;
  WFST::out_arc_brief(*fstout);
  if ( flags['H'] )
    //*fstout << WFST::out_arc_per_line;
  WFST::out_arc_per_line(*fstout);
  else
    //*fstout << WFST::out_state_per_line;
  WFST::out_state_per_line(*fstout);

}

static void printSeq(Alphabet *a,int *seq,int maxSize) {

  for ( int i = 0 ; i < maxSize && seq[i] != 0; ++i) {
    if (i>0)
      cout << ' ';

    cout << (*a)[seq[i]];

  }
}

template <class T>
void readParam(T *t, char *from, char sw) {
  istringstream is(from);
  is >> *t;
  if ( is.fail() ) {
    Config::warn() << "Expected a number after -" << sw << " switch, (instead got \'" << from << "\' - as a number, " << *t << ").\n";
    exit(-1);
  }
}

int isSpecial(const char* psz) {
  if ( !(*psz == '*') )
    return 0;
  while ( *++psz ) ;
  return *--psz == '*';
}

void outWithoutQuotes(const char *str, ostream &out) {
  if ( *str != '\"') {
    out << str;
    return;
  }
  const char *psz = str;
  while ( *++psz ) ;
  if ( *--psz == '\"' ) {
    while ( ++str < psz )
      out << *str;
  } else
    out << str;
}


void printPath(bool *flags,const List<PathArc> *pli) {
  Weight w = 1.0;
  const char * outSym;

  for (List<PathArc>::const_iterator li=pli->const_begin(),end=pli->const_end(); li != end; ++li ) {
    const WFST *f=li->wfst;
    if ( flags['O'] || flags['I'] ) {
      if ( flags['O'] )
        outSym = f->outLetter(li->out);
      else
        outSym = f->inLetter(li->in);
      if ( !(flags['E'] && isSpecial(outSym)) ) {
        if ( flags['Q'] )
          outWithoutQuotes(outSym, cout);
        else
          cout << outSym;
        cout << " ";
      }
    } else {
      cout << *li << " ";
    }
    w = w * li->weight;
  }
  if ( !flags['W'] )
    cout << w;
  cout << "\n";

}



void usageHelp(void);
void WFSTformatHelp(void);


int
#ifdef _MSC_VER
__cdecl
#endif
main(int argc, char *argv[]){
  INITLEAK;
#ifdef _MSC_VER
#ifdef MEMDEBUG
  //int tmpFlag = CrtSetDbgFlag(CRTDBGREPORTFLAG);
  //tmpFlag |= CRTDBGCHECKALWAYSDF;
  //CrtSetDbgFlag(tmpFlag);

#endif
#endif
#ifdef MARCU
  argc=sizeof(MarcuArgs)/sizeof(char *);
  argv=MarcuArgs;
#endif
  if ( argc == 1 ) {
    usageHelp();
    return 0;
  }
  int i;
  bool flags[256];
  for ( i = 0 ; i < 256 ; ++i ) flags[i] = 0;
  char *pc, **parm = NEW char *[argc-1];
  unsigned int seed = (unsigned int )std::time(NULL);
  int nParms = 0;
  int kPaths = 0;
  int thresh = 32;
  Weight converge = 1E-4;
  Weight converge_pp_ratio = .999;
  bool converge_pp_flag = false;
  int convergeFlag = 0;
  Weight smoothFloor;
  Weight prune;
  Weight prod_prob=1;
  int n_pairs=0;
  WFST::NormalizeMethod norm_method = WFST::CONDITIONAL;
  int pruneFlag = 0;
  int floorFlag = 0;
  bool seedFlag = false;
  int max_states = WFST::UNLIMITED;
  Weight keep_path_ratio;
  keep_path_ratio.setInfinity();
  bool wrFlag = false;
  bool msFlag = false;
  bool learning_rate_growth_flag = false;
  FLOAT_TYPE learning_rate_growth_factor=(FLOAT_TYPE)1.;
  int nGenerate = 0;
  int maxTrainIter = 256;
#define DEFAULT_MAX_GEN_ARCS 1000
  int maxGenArcs = 0;
  int labelStart = 0;
  int labelFlag = 0;
        bool rrFlag=false;
        int ranRestarts = 0;
  bool isInChain;
  ostream *fstout = &cout;

  std::ios_base::sync_with_stdio(false);

  for ( i = 1 ; i < argc ; ++i ) {
    if ((pc=argv[i])[0] == '-' && pc[1] != '\0' && !learning_rate_growth_flag && !convergeFlag && !floorFlag && !pruneFlag && !labelFlag && !converge_pp_flag && !wrFlag && !msFlag)
      while ( *(++pc) ) {
        if ( *pc == 'k' )
          kPaths = -1;
        else if ( *pc == '!' )
                                  rrFlag=true;
        else if ( *pc == 'o' )
          learning_rate_growth_flag=true;
        else if ( *pc == 'X' )
          converge_pp_flag=true;
        else if ( *pc == 'w' )
          wrFlag=true;
        else if ( *pc == 'z' )
          msFlag=true;
        else if ( *pc == 'R' )
          seedFlag=true;
        else if ( *pc == 'F' )
          fstout = NULL;
        else if ( *pc == 'T' )
          thresh = -1;
        else if ( *pc == 'e' )
          convergeFlag = 1;
        else if ( *pc == 'f' )
          floorFlag = 1;
        else if ( *pc == 'p' )
          pruneFlag = 1;
        else if ( *pc == 'g' || *pc == 'G' )
          nGenerate = -1;
        else if ( *pc == 'M' )
          maxTrainIter = -1;
        else if ( *pc == 'L' )
          maxGenArcs = -1;
        else if ( *pc == 'N' )
          labelFlag = 1;
        else if ( *pc == 'j' )
          norm_method = WFST::JOINT;
        else if ( *pc == 'u' )
          norm_method = WFST::NONE;
        flags[*pc] = 1;
      }
    else
      if ( labelFlag ) {
        labelFlag = 0;
        readParam(&labelStart,argv[i],'N');
      } else if ( converge_pp_flag ) {
        converge_pp_flag = false;
        readParam(&converge_pp_ratio,argv[i],'X');
      } else if ( learning_rate_growth_flag ) {
        learning_rate_growth_flag = false;
        readParam(&learning_rate_growth_factor,argv[i],'o');
        if (learning_rate_growth_factor < 1)
          learning_rate_growth_factor=1;
      } else if (rrFlag) {
        rrFlag=false;
        readParam(&ranRestarts,argv[i],'!');
      } else if (seedFlag) {
        seedFlag=false;
        readParam(&seed,argv[i],'R');
      } else if ( wrFlag ) {
        readParam(&keep_path_ratio,argv[i],'w');
        if (keep_path_ratio < 1)
          keep_path_ratio = 1;
        wrFlag=false;
      } else if ( msFlag ) {
        readParam(&max_states,argv[i],'z');
        if ( max_states < 1 )
          max_states = 1;
        msFlag=false;
      } else if ( kPaths == -1 ) {
        readParam(&kPaths,argv[i],'k');
        if ( kPaths < 1 )
          kPaths = 1;
      } else if ( nGenerate == -1 ) {
        readParam(&nGenerate,argv[i],'g');
        if ( nGenerate < 1 )
          nGenerate = 1;
      } else if ( maxTrainIter == -1 ) {
        readParam(&maxTrainIter,argv[i],'M');
        if ( maxTrainIter < 1 )
          maxTrainIter = 1;
      } else if ( maxGenArcs == -1 ) {
        readParam(&maxGenArcs,argv[i],'L');
        if ( maxGenArcs < 0 )
          maxGenArcs = 0;
      } else if ( thresh == -1 ) {
        readParam(&thresh,argv[i],'T');
        if ( thresh < 0 )
          thresh = 0;
      } else if ( convergeFlag ) {
        convergeFlag = 0;
        readParam(&converge,argv[i],'e');
      } else if ( floorFlag ) {
        floorFlag = 0;
        readParam(&smoothFloor,argv[i],'f');
      } else if ( pruneFlag ) {
        pruneFlag = 0;
        readParam(&prune,argv[i],'p');
      } else if ( fstout == NULL ) {
        fstout = NEW ofstream(argv[i]);
        setOutputFormat(flags,fstout);
        if ( !*fstout ) {
          Config::warn() << "Could not create file " << argv[i] << ".\n";
          return -8;
        }
      } else
        parm[nParms++] = argv[i];
  }
  bool prunePath = flags['w'] || flags['z'];
  srand(seed);
  setOutputFormat(flags,&cout);
  setOutputFormat(flags,&cerr);
  WFST::setIndexThreshold(thresh);
  if ( flags['h'] ) {
    WFSTformatHelp();
    cout << endl << endl;
    usageHelp();
    return 0;
  }
  if (flags['V']){
    std::cout << "Carmel Version: " << VERSION ;
    std::cout << ". Copyright C 2000, the University of Southern California.\n";
    return 0 ;
  }
  if ( flags['b'] && kPaths < 1 )
    kPaths = 1;
  istream **inputs, **files, **newed_inputs;
  char **filenames;
  int nInputs,nChain;
#ifdef MARCU
  initModels();
  int nModels=(int)Models.size();
#endif
  if ( flags['s'] ) {
    nChain=nInputs = nParms + 1;
#ifdef MARCU
    nChain+=(int)Models.size();
#endif
    newed_inputs = inputs = NEW istream *[nInputs];
    filenames = NEW char *[nChain];
    if ( flags['r'] ) {
      inputs[nParms] = &cin;
      filenames[nParms] = "stdin";
      for ( i = 0 ; i < nParms ; ++i )
        filenames[i] = parm[i];
      files = inputs;
    } else {
      inputs[0] = &cin;
      filenames[0] = "stdin";
      for ( i = 0 ; i < nParms ; ++i )
        filenames[i+1] = parm[i];
      files = inputs + 1;
    }
  } else {
    nChain=nInputs = nParms;
    newed_inputs = inputs = NEW istream *[nInputs];
    files = inputs;
    filenames = parm;
  }
  istream *pairStream = NULL;
  if ( flags['t'] )
    flags['S'] = 1;
  //(flags['S'] ||
  if ( nChain < 1 || flags['A'] && nInputs < 2) {
    Config::warn() << "No inputs supplied.\n";
    return -12;
  }
  for ( i = 0 ; i < nParms ; ++i ) {
    //    if(parm[i][0]=='-' && parm[i][1] == '\0')
    //              files[i] = &cin;
    //      else
    files[i] = NEW ifstream(parm[i]);
#ifdef DEBUG
    //Config::debug() << "Created file " << i << " from " << parm[i] << " & " << files[i] <<"\n";
#endif
    if ( !*files[i] ) {
      Config::warn() << "File " << parm[i] << " could not be opened for input.\n";
      for ( int j = 0 ; j <=i ; ++j )
        delete files[j];
      delete[] parm;
      delete[] inputs;
      return -9;
    }
  }
  if ( flags['S'] ) {
    flags['b'] = flags['x'] = flags['y'] = 0;
    kPaths = 0;
    if (nInputs > 1) {
      --nInputs;
      --nChain;
      if ( flags['r'] )
        pairStream = inputs[nInputs];
      else {
        pairStream = inputs[0];
        ++filenames;
        ++inputs;
      }
    }
    if ( flags['s'] )
      ++files;
    else
      --nParms;
  }
  WFST *chainMemory = (WFST*)::operator new(nChain * sizeof(WFST));
  WFST *chain = chainMemory;
#ifdef MARCU
  chain+=nModels;
#endif
  int nTarget = -1; // chain[nTarget] is the linear acceptor built from input sequences
  istream *line_in=NULL;
  if ( flags['i'] || flags['b']||flags['P']) {// flags['P'] similar to 'i' but instead of simple transducer, produce permutation lattice.
    if ( flags['r'] )
      nTarget = nInputs-1;
    else
      nTarget = 0;
    line_in=inputs[nTarget];
  }
  for ( i = 0 ; i < nInputs ; ++i ) {
    if ( i != nTarget ) {
      PLACEMENT_NEW (&chain[i]) WFST(*inputs[i],!flags['K']);
      if ( !flags['m'] && nInputs > 1 )
        chain[i].unNameStates();
      if ( inputs[i] != &cin ) {
#ifdef DEBUG
        //Config::debug() << "Deleting file " << i << " & " << inputs[i] <<"\n";
#endif

        delete inputs[i];
      }
      if ( !(chain[i].valid()) ) {
        Config::warn() << "Bad format of transducer file: " << filenames[i] << "\n";
        //        for ( j = i+1 ; j < nInputs ; ++j )
        //          if ( inputs[i] != &cin )
        //            delete inputs[i];
        return -2;
      }
    }
  }

  string buf ;

  WFST *result = NULL;
  WFST *weightSource = NULL; // assign waits from this transducer to result by tie groups
  if ( flags['A'] ) {
    --nInputs;
    --nChain;
    if ( !flags['r'] ) {
      weightSource = chain;
      ++chain;
    } else
      weightSource = chain + nInputs;
  }
  int input_lineno=0;

#ifdef MARCU
  //chain,inputs,filenames,nTarget,nInputs
  for (i=0,j=nModels;i<nInputs;++i,++j)
    filenames[j]=filenames[i];
  chain-=nModels;
  nTarget+=nModels;
  for (j=0;j<nModels;++j) {
    PLACEMENT_NEW (&chain[j]) WFST(Models[j],!flags['K']);
    filenames[j] = "Models.builtin";
  }
#endif

  for ( ; ; ) { // input transducer from string line reading loop
    if (nTarget != -1) { // if to construct a finite state from input
	  if ( !*line_in ) {
fail_ntarget: 
		Config::warn() << "No lines of input provided.\n";
		PLACEMENT_NEW (&chain[nTarget])WFST(); break;
	  }
        
      *line_in >> ws;
      getline(*line_in,buf);

	  if ( !*line_in ) 		
		goto fail_ntarget;
			 
      if ( input_lineno != 0 )
        chain[nTarget].~WFST();
      if (flags['P']){ // need a permutation lattice instead
        int length ;
        PLACEMENT_NEW (&chain[nTarget]) WFST(buf.c_str(),length,1);
      } else // no permutation, just need input acceptor
        PLACEMENT_NEW (&chain[nTarget]) WFST(buf.c_str());
      CHECKLEAK(input_lineno);
      ++input_lineno;

      if (!flags['q'])
        Config::log() << "Input line " << input_lineno << ": " << buf.c_str();
#ifdef  DEBUGCOMPOSE
      Config::debug() << "\nprocessing input line " << input_lineno << ": " << buf.c_str() << " storing in chain[" << nTarget << "]\n";
#endif
      if ( !(chain[nTarget].valid()) ) {
        Config::warn() << "Couldn't handle input line: " << buf << "\n";
        return -3;
      }
    }

#define PRUNE do { \
        if ( flags['p'] ) \
          result->pruneArcs(prune); \
                if ( prunePath ) \
                  result->prunePaths(max_states,keep_path_ratio); \
       } while(0)

#define MINIMIZE do { \
        if ( flags['C'] ) \
          result->consolidateArcs(); \
        if ( !flags['d'] ) \
                result->reduce(); \
       } while(0)

    bool r=flags['r'];
    result = (r ? &chain[nChain-1] :&chain[0]);
    bool first=true;
    MINIMIZE;
    if (nInputs < 2)
      PRUNE;
#ifdef  DEBUGCOMPOSE
    Config::debug() << "\nStarting composition chain: result is chain[" << (int)(result-chain) <<"]\n";
#endif

    for ( i = (r ? nChain-2 : 1); (r ? i >= 0 : i < nChain) && result->valid() ; (r ? --i : ++i),first=false ) { // composition loop

#ifdef  DEBUGCOMPOSE
      Config::debug() << "----------\ncomposing result with chain[" << i<<"] into next\n";
#endif
      // composition happens here:
      WFST *next = NEW WFST((r ? chain[i] : *result), (r ? *result : chain[i]), flags['m'], flags['a']);
#ifndef NODELETE
      //      if (nTarget != -1) {
      //        (r ? next->stealOutAlphabet(*result) : next->stealInAlphabet(*result));
#ifdef  DEBUGCOMPOSE
      //        Config::debug() << "result will be going away - take its alphabet\n";
#endif
      //      }

      //      if (nTarget != -1) {
      // &&(result !=  &chain[nTarget]) ){
      // the above condition was removed - why?  because in -i -P or -b mode, the input transducer needs to be deleted.  was a memory leak.  we protect against deleting the chain twice at the end of main by checking nTarget
#ifdef DEBUGCOMPOSE
      Config::debug() << "deleting result and replacing it with next\n";
#endif
      if (!first)
        delete result;
      //      }
#endif
      result = next;

      int q_states=result->size();
      int q_arcs=result->numArcs();
      if (!flags['q'])
        Config::log() << "\n\t(" << result->size() << " states / " << result->numArcs() << " arcs";
#ifdef  DEBUGCOMPOSE
      Config::debug() <<"stats for the resulting composition for chain[" << i << "]\n";
      Config::debug() << "Number of states in result: " << result->size() << std::endl;
      Config::debug() << "Number of arcs in result: " << result->numArcs() << std::endl;
      //      Config::debug() << "Number of paths in result (without taking cycles): " << result->numNoCyclePaths() << std::endl;
      //sleep(100);
      // Config::debug() << "woke up\n";
#endif

      if ( !result->valid() ) {
        Config::warn() << ")\nEmpty or invalid result of composition with transducer " << filenames[i] << ".\n";
        for ( i = 0 ; i < kPaths ; ++i ) {
          if ( !flags['W'] )
            cout << 0;
          cout << "\n";
        }

        goto nextInput;
      }
      bool finalcompose = i == (r ? 0 : nChain-1);
      MINIMIZE;
      if (!flags['q'] && (q_states != result->size() || q_arcs !=result->numArcs()))
        Config::log()  << " reduce-> " << result->size() << "/" << result->numArcs();
      if (!(kPaths>0 && finalcompose)) { // pruning is at least as hard (and includes) finding best paths already; why duplicate effort?
        q_states=result->size();
        q_arcs=result->numArcs();
        PRUNE;
        if (!flags['q'] && (q_states != result->size() || q_arcs !=result->numArcs()))
          Config::log()  << " prune-> " << result->size() << "/" << result->numArcs();
      }
      if (!flags['q'])
        Config::log() << ")";
    }
    if (!flags['q'] && nChain > 1 )
      Config::log() << std::endl;

#ifdef  DEBUGCOMPOSE
    Config::debug() << "done chain of compositions  .. now processing result\n";
#endif

    if ( flags['v'] )
      result->invert();
        if ( flags['1'] )
          result->randomScale();
    if ( flags['n'] )
      result->normalize(norm_method);
    if ( flags['A'] ) {
      Assert(weightSource);
      result->assignWeights(*weightSource);
    }
    if ( flags['N'] ) {
      if (labelStart > 0)
        result->numberArcsFrom(labelStart);
      else if ( labelStart == 0 )
        result->lockArcs();
      else
        result->unTieGroups();
    }
    if ( kPaths > 0  ) {
      int nGoodPaths = 0;
      if ( result->valid() ) {
        List <List<PathArc> > *bestPaths = result->bestPaths(kPaths);
        Assert(bestPaths);
        for ( List<List<PathArc> >::const_iterator pli=bestPaths->const_begin(),end=bestPaths->const_end()  ; pli != end; ++pli, ++nGoodPaths )
          printPath(flags,&*pli);

        delete bestPaths;
      }
      for ( int fill = 0 ; fill < kPaths - nGoodPaths ; ++fill ) {
        if ( !flags['W'] )
          cout << 0;
        cout << "\n";

      }

    } else if ( flags['x'] ) {
      result->listAlphabet(cout, 0);
    } else if ( flags['y'] ) {
      result->listAlphabet(cout, 1);
    } else if ( flags['c'] ) {
      cout << "Number of states in result: " << result->size() << std::endl;
      cout << "Number of arcs in result: " << result->numArcs() << std::endl;
      cout << "Number of paths in result (without taking cycles): " << result->numNoCyclePaths() << std::endl;
    }
    if ( flags['t'] )
      flags['S'] = 0;
    if ( !flags['b'] ) {
      if ( flags['S'] ) {
        n_pairs=0;
        if (pairStream) {
          for ( ; ; ) {
            getline(*pairStream,buf);
            if ( !*pairStream )
              break;
            ++input_lineno;
            List<int> *inSeq = result->symbolList(buf.c_str(), 0);
            if ( !inSeq ) {
              Config::warn() << "Input sequence: " << buf << " on line " << input_lineno << " contains symbols not in the alphabet.\n";
              return -22;
            }
            getline(*pairStream,buf);
            ++input_lineno;
            if ( !*pairStream )
              break;
            List<int> *outSeq = result->symbolList(buf.c_str(), 1);
            if ( !outSeq ) {
              Config::warn() << "Output sequence: " << buf << " on line " << input_lineno << " contains symbols not in the alphabet.\n";
              return -21;
            }
            Weight prob=result->sumOfAllPaths(*inSeq, *outSeq);
            ++n_pairs;
            prod_prob*=prob;
            cout << prob << std::endl;
            delete inSeq;
            delete outSeq;
          }
        } else {
          List<int> empty_list;
          n_pairs=1;
          cout << (prod_prob=result->sumOfAllPaths(empty_list, empty_list)) << std::endl;
        }
      } else if ( flags['t'] ) {
        FLOAT_TYPE weight;
        result->trainBegin(norm_method,flags['U'],smoothFloor);
        if (pairStream) {
          for ( ; ; ) {
            weight = 1;
            getline(*pairStream,buf);
            if ( !*pairStream )
              break;
            if ( isdigit(buf[0]) || buf[0] == '-' || buf[0] == '.' ) {
              istringstream w(buf.c_str());
              w >> weight;
              if ( w.fail() ) {
                Config::warn() << "Bad training example weight: " << buf << std::endl;
                continue;
              }
              getline(*pairStream,buf);
              if ( !*pairStream )
                break;
            }
            List<int> *inSeq = result->symbolList(buf.c_str(), 0);
            if ( !inSeq ) {
              Config::warn() << "Input sequence: " << buf << " contains symbols not in the alphabet.\n";
              return -22;
            }
            getline(*pairStream,buf);
            if ( !*pairStream )
              break;
            List<int> *outSeq = result->symbolList(buf.c_str(), 1);
            if ( !outSeq ) {
              Config::warn() << "Output sequence: " << buf << " contains symbols not in the alphabet.\n";
              return -21;
            }
            result->trainExample(*inSeq, *outSeq, weight);
            delete inSeq;
            delete outSeq;
          }
        } else {
          List<int> empty_list;
          result->trainExample(empty_list, empty_list, 1.0);
        }
        result->trainFinish(converge, converge_pp_ratio, maxTrainIter, learning_rate_growth_factor, norm_method, ranRestarts);
      } else if ( nGenerate > 0 ) {
        MINIMIZE;
        //        if ( !flags['n'] )
        //        result->normalize(norm_method);
        if ( maxGenArcs == 0 )
          maxGenArcs = DEFAULT_MAX_GEN_ARCS;
        if ( flags['G'] ) {
          for (int i=0; i<nGenerate; ) {
            List<PathArc> l;
            if (result->randomPath(&l) != -1) {
              printPath(flags,&l);
              ++i;
            }
          }
        } else {
          int maxSize = maxGenArcs+1;
          int *inSeq = NEW int[maxSize];
          int *outSeq = NEW int[maxSize];
          for ( int s = 0 ; s < nGenerate ; ++s ) {

            while ( !result->generate(inSeq, outSeq, 0, maxGenArcs) ) ;
            printSeq(result->in,inSeq,maxGenArcs);
            cout << std::endl;
            printSeq(result->out,outSeq,maxGenArcs);
            cout << std::endl;
            /*for ( i = 0 ; i < maxSize ; ) {
              if (outSeq[i] > 0)
              cout << result->outLetter(outSeq[i]);
              ++i;
              if ( i < maxSize && outSeq[i] > 0 )
              cout << ' ';
              }
              cout << std::endl;
            */
          }
          delete[] inSeq;
          delete[] outSeq;
        }
      }




      if ( ( !flags['k'] && !flags['x'] && !flags['y'] && !flags['S']) && !flags['c'] && !flags['g'] && !flags['G'] || flags['F'] ) {
        PRUNE;
        if ( flags['t'] )
          if (flags['p'] || prunePath) {
            result->normalize(norm_method);
          }
        MINIMIZE;

        if ( flags ['Y'] )
          result->writeGraphViz(*fstout);
        else
          *fstout << *result;
      }
    }
  nextInput:

#ifndef NODELETE

    // Now delete the compostion result to free up memory, this is important
    // when you are doing batch compostions (option -b).
    // But make sure  first that you are not deleting one of the main
    // WFSTs. That is why we check if the result is one of them and if it is
    // we don't delete it.
    isInChain = false ;
    for (int i = 0 ; i < nChain ; i++){
      if (result == &chain[i])
        isInChain = true ;
    }
    if (!isInChain){
#ifdef  DEBUGCOMPOSE
      Config::debug() << "deleting result at end of processing\n";
#endif
      delete result ;
    }
#ifdef  DEBUGCOMPOSE
    else
      Config::debug() << "can't delete result because it is part of chain .. \n";
#endif
#endif
    if ( !flags['b'] )
      break;
  } // end of all input
  if ( flags['S'] && input_lineno > 0) {
    Config::log() << "Corpus probability=" << prod_prob << " ; Model perplexity(N=" << n_pairs <<")=" << root(prod_prob,(FLOAT_TYPE)n_pairs).invert() << std::endl;
  }

#ifndef NODELETE
  for ( i = 0 ; i < nChain ; ++i )
    chainMemory[i].~WFST();
  //  if ( flags['A'] )
  //    chainMemory[i].~WFST();
#endif
  ::operator delete(chainMemory);
  delete[] parm; // alias filenames unless -s
  if(flags['s'])
    delete[] filenames;
  if(nTarget != -1) {
    if(line_in != &cin)
      delete line_in; // rest were deleted after transducers were read
  }
  if (pairStream != &cin)
    delete pairStream;
  delete[] newed_inputs;
  if ( fstout != &cout )
    delete fstout;
  return 0;
}

void usageHelp(void)
{
  cout << "usage: carmel [switches] [file1 file2 ... filen]\n\ncomposes a seq";
  cout << "uence of weighted finite state transducers and writes the\nresul";
  cout << "t to the standard output.\n\n-l (default)\tleft associative comp";
  cout << "osition ((file1*file2) * file3 ... )\n-r\t\tright associative co";
  cout << "mposition (file1 * (file2*file3) ... )\n-s\t\tthe standard input";
  cout << " is prepended to the sequence of files (for\n\t\tleft associativ";
  cout << "e composition), or appended (if right\n\t\tassociative)\n-i\t\tt";
  cout << "he first input (depending on associativity) is interpreted as\n\t";
  cout << "\ta space-separated sequence of symbols, and translated into a\n";
  cout << "\t\ttransducer accepting only that sequence\n";
  cout << "-P Similar to (-i) but instead of building an acceptor with a\n\t\t";
  cout << "single arc, construct a permutaion lattice that accepts the\n\t\t";
  cout << "input in all possible reorderings.\n-k n\t\tthe n best ";
  cout << "paths through the resulting transducer are written\n\t\tto the s";
  cout << "tandard output in lieu of the transducer itself\n-b\t\tbatch com";
  cout << "postion - reads the sequence of transducers into\n\t\tmemory, ex";
  cout << "cept the first input (depending on associativity),\n\t\twhich co";
  cout << "nsists of sequences of space-separated input symbols\n\t\t(as in";
  cout << " -i) separated by newlines.  The best path(s) through\n\t\tthe r";
  cout << "esult of each composition are written to the standard\n\t\toutpu";
  cout << "t, one per line, in the same order as the inputs that\n\t\tgener";
  cout << "ated them\n-S\t\tas in -b, the input (file or stdin) is a newlin";
  cout << "e separated\n\t\tlist of symbol sequences, except that now the o";
  cout << "dd lines are\n\t\tinput sequences, with the subsequent sequence ";
  cout << "being the\n\t\tcorresponding output sequence\n\t\tthis command s";
  cout << "cores the input / output pairs by adding the sum\n\t\tof the wei";
  cout << "ghts of all possible paths producing them, printing\n\t\tthe wei";
  cout << "ghts one per line if -i is used, it will apply to the\n\t\tsecon";
  cout << "d input, as -i consumes the first\n-n\t\tnormalize the weights o";
  cout << "f arcs so that for each state, the\n\t\tweights all of the arcs ";
  cout << "with the same input symbol add to one\n-t\t\tgiven pairs of inpu";
  cout << "t/output sequences, as in -S, adjust the\n\t\tweights of the tra";
  cout << "nsducer so as to approximate the conditional\n\t\tdistribution o";
  cout << "f the output sequences given the input sequences\n\t\toptionally";
  cout << ", an extra line preceeding an input/output pair may\n\t\tcontain";
  cout << " a floating point number for how many times the \n\t\tinput/outp";
  cout << "ut pair should count in training (default is 1)\n-e w\t\tw is th";
  cout << "e convergence criteria for training (the minimum\n\t\tchange in ";
  cout << "an arc\'s weight to trigger another iteration) - \n\t\tdefault w";
  cout << " is 1E-4 (or, -4log)\n-X w\t\tw is a perplexity convergence ratio between 0 and 1,\n\t\twith 1 being the strictest (default w=.999)\n-f w\t\tw is a per-training example floor weight used for train";
  cout << "ing,\n\t\tadded to the counts for all arcs, immediately";
  cout << " before normalization -\n\t\t(this implements so-called \"Dirichlet prior\" smoothing)";
  cout << "\n-U\t\tuse the initial weights of non-locked arcs as per-example prior counts\n\t\t(in the same way as, and in addition to, -f)";
  cout << "\n-M n\t\tn is th";
  cout << "e maximum number of training iterations that will be\n\t\tperfor";
  cout << "med, regardless of whether the convergence criteria is\n\t\tmet ";
  cout << "- default n is 256\n-x\t\tlist only the input alphabet of the tr";
  cout << "ansducer to stdout\n-y\t\tlist only the output alphabet of the t";
  cout << "ransducer to stdout\n-c\t\tlist only statistics on the transduce";
  cout << "r to stdout\n-F filename\twrite the final transducer to a file (";
  cout << "in lieu of stdout)\n-v\t\tinvert the ";
  cout << "resulting transducer by swapping the input and\n\t\toutput symbo";
  cout << "ls \n-d\t\tdo not reduce (eliminate dead-end states)";
  cout << " created\n-C\t\tconsolidate arcs with same source, destination, ";
  cout << "input and\n\t\toutput, with a total weight equal to the sum (cla";
  cout << "mped to a\n\t\tmaximum weight of one)\n-p w\t\tprune (discard) a";
  cout << "ll arcs with weight less than w";
  cout << "\n-w w\t\tprune states and arcs only used in paths w times worse\n\t\tthan the best path (1 means keep only best path, 10 = keep paths up to 10 times weaker)";
  cout <<"\n-z n\t\tkeep at most n states (those used in highest-scoring paths)";
  cout << "\n-g n\t\tstochastically generate";
  cout << " n input/output pairs by following\n\t\trandom paths (first choosing an input symbol with uniform\n\t\tprobability, then using the weights to choose an output symbol\n\t\tand destination) from the in";
  cout << "itial state to the final state\n\t\toutput is in the same for";
  cout << "m accepted in -t and -S.\n\t\tTraining a transducer on its own -g output should be a no-op.\n-G n\t\tstochastically generate n paths by randomly picking an arc\n\t\tleaving the current state, by joint normalization\n\t\tuntil the final state is reached.\n\t\tsame output format as -k best paths\n-R n\t\tUse n as the random seed for repeatable -g and -G results\n\t\tdefault seed = current time\n-L n\t\twhile generating input/output p";
  cout << "airs with -g or -G, give up if\n\t\tfinal state isn't reached after n steps (default n=1000)\n-T n\t\tduring composit";
  cout << "ion, index arcs in a hash table when the\n\t\tproduct of the num";
  cout << "ber of arcs of two states is greater than n \n\t\t(by default, n";
  cout << " = 32)\n-N n\t\tassign each arc in the result transducer a uniq";
  cout << "ue group number\n\t\tstarting at n and counting up.  If n is 0 (";
  cout << "the special group\n\t\tfor unchangeable arcs), all the arcs are ";
  cout << "assigned to group 0\n\t\tif n is negative, all group numbers are";
  cout << " removed\n-A\t\tthe weights in the first transducer (depending o";
  cout << "n -l or -r, as\n\t\twith -b, -S, and -t) are assigned to the res";
  cout << "ult, by arc group\n\t\tnumber.  Arcs with group numbers for whic";
  cout << "h there is no\n\t\tcorresponding group in the first transducer a";
  cout << "re removed\n-m\t\tgive meaningful names to states created in com";
  cout << "position\n\t\trather than just numbers\n-a\t\tduring composition";
  cout << ", keep the identity of matching arcs from\n\t\tthe two transduce";
  cout << "rs separate, assigning the same arc group\n\t\tnumber to arcs in";
  cout << " the result as the arc in the transducer it\n\t\tcame from.  Thi";
  cout << "s will create more states, and possibly less\n\t\tarcs, than the";
  cout << " normal approach, but the transducer will have\n\t\tequivalent p";
  cout << "aths.\n-h\t\thelp on transducers, file formats\n";
  cout << "-V\t\tversion number\n-u\t\tDon't normalize outgoing arcs for each input during training;\n\t\ttry -tuM 1 to see forward-backward counts for arcs" ;
  cout << "\n-j\t\tPerform joint rather than conditional normalization";
  cout << "\n-Y\t\tWrite transducer to GraphViz .dot file\n\t\t(see http://www.research.att.com/sw/tools/graphviz/)";
  cout << "\n-q\t\tSuppress computation status messages (quiet!)";
  cout << "\n-K\t\tAssume state names are integer indexes (when the final state is an integer)";
  cout << "\n-o g\t\tUse learning rate growth factor g (>= 1) (default=1)";
  cout << "\n-1\t\trandomly scale weights (of unlocked arcs) after composition uniformly by (0..1]";
        cout << "\n-! n\t\tperform n additional random initializations of arcs for training, keeping the lowest perplexity";
  cout << "\n\n";
  cout << "some formatting switches for paths from -k or -G:\n\t-I\tshow input symbols ";
  cout << "only\n\t-O\tshow output symbols only\n\t-E\tif -I or -O is speci";
  cout << "fied, omit special symbols (beginning and\n\t\tending with an as";
  cout << "terisk (e.g. \"*e*\"))\n\t-Q\tif -I or -O is specified, omit out";
  cout << "ermost quotes of symbol names\n\t-W\tdo not show weights for pat";
  cout << "hs";
  cout << "\n\nWeight output format switches\n\t\t(by default, small/large weights are written as logarithms):";
  cout << "\n\t-B\tWrite weights as their base 10 log (default is ln, e.g.\n\t\t'-5ln' signifies e^(-5))";
  cout << "\n\t-Z\tWrite weights in logarithm form always, e.g. '-10ln',\n\t\texcept for 0, which is written simply as '0'";
  cout << "\n\t-D\tWrite weights as reals always, e.g. '1.234e-200'";
  cout << "\n\nTransducer output format switches:";
  cout << "\n\t-H\tOne arc per line (by default one state and all its arcs per line)";
  cout << "\n\t-J\tDon't omit output=input or Weight=1";

  cout << "\n\nConfused?  Think you\'ve found a bug?  If all else fails, ";
  cout << "e-mail graehl@isi.edu or knight@isi.edu\n\n";
}

void WFSTformatHelp(void)
{
  cout << "A weighted finite state transducer (WFST) is a directed graph wi";
  cout << "th weighted\narcs, and an input and output label on each arc.  E";
  cout << "ach vertex is called a\n\"state\", and one state is designated a";
  cout << "s the initial state and another as the\nfinal state.  Every path";
  cout << " from the initial state to the final state represents\na transdu";
  cout << "ction from the concatenation of the input labels of arcs along t";
  cout << "he\npath, to the concatenation of the output labels, with a weig";
  cout << "ht equal to the\nproduct of the weights of each transition taken";
  cout << ".  The weight of a transduction\nmay represent, for instance, th";
  cout << "e conditional probability of the output\nsequence given the inpu";
  cout << "t sequence.  The actual weight that should be assigned\nto a tra";
  cout << "nsduction is, more accurately, the sum of the weights of all pos";
  cout << "sible\npaths which emit the input:output sequence, but it is mor";
  cout << "e convenient to\nsearch for the best path, so that the sum is ap";
  cout << "proximated by the maximum.  A\nspecial \"empty\" symbol, when us";
  cout << "ed as an input or output label, disappears in\nthe concatenation";
  cout << "s.\n\nTwo transductions may be composed, meaning that the weight";
  cout << " of any input:output\npair is the sum over all intermediate poss";
  cout << "ibilities of the product of the\nweight of input:intermediate in";
  cout << " the first and the weight of\nintermediate:output in the second.";
  cout << "  If the transductions represent conditional\nprobabilities, and";
  cout << " the intermediate possibilities represent a partition of\nevents";
  cout << " such that input and output are independent given the intermedia";
  cout << "te\npossibilities, then the conditional probability of output gi";
  cout << "ven input is given\nby the composition of the first transducer, ";
  cout << "which contains the probabilities\np(input|intermediates) and the";
  cout << " second transducer, which contains the\nprobabilities p(intermed";
  cout << "iates|output).  Given two WFST representing\ntransductions, anot";
  cout << "her WFST representing the composition of the two\ntransductions ";
  cout << "can be easily created (with up to the product of the number of\n";
  cout << "states).\n\nThis program accepts parenthesized lists of states a";
  cout << "nd arcs in a variety of\nformats.  Whitespace can be used freely";
  cout << " between symbols.  States are named by\nany string of characters";
  cout << " (except parentheses) delimited by whitespace, or\ncontained in ";
  cout << "bounding quotes or asterisks (\"state 1\" or *initial state*, fo";
  cout << "r\nexample).  Input/output symbols are of the same format, excep";
  cout << "t they must not\nbegin with a number, decimal point, or minus.  ";
  cout << "Input/output symbols bounded by\nasterisks are intended to be sp";
  cout << "ecial symbols.  Only one, *e*, the empty\nsymbol, is currently t";
  cout << "reated differently by the program.  Special symbols\nare not cas";
  cout << "e sensitive (converted to lowercase on input).  Quoted symbol na";
  cout << "mes\nmay contain internal quotes so long as they are escaped by ";
  cout << "a backslash\nimmediately preceding them (e.g. \"\\\"hello\\\"\")";
  cout << ".  Symbols should not be longer\nthan 4000 characters.  Weights ";
  cout << "are floating point numbers, optionally followed\nimmediately by ";
  cout << "the letters \'log\', in which case the number is the base 10\nlo";
  cout << "garithm of the actual weight (logarithms are used internally to ";
  cout << "avoid\nunderflow).\n\nEvery transducer file begins with the name";
  cout << " of the final state, and is followed\nby a list of arcs.  No exp";
  cout << "licit list of states is needed; if a state name\noccurs as the s";
  cout << "ource or destination of an arc, it is created.  Each\nparenthesi";
  cout << "zed expression of the form describes zero or more arcs (here the";
  cout << "\nasterisk means zero or more repetitions of the parenthesized e";
  cout << "xpression to the\nleft) :\n\n(source-state (destination-state in";
  cout << "put output weight)*)\n(source-state (destination-state (input ou";
  cout << "tput weight)*)*)\n\nif right parentheses following \"weight\" is";
  cout << " immediately preceded by an\nexclamation point (\'!\'), then tha";
  cout << "t weight will be locked and unchanged by\nnormalization or train";
  cout << "ing (however, the arc may still be eliminated by\nreduction (abs";
  cout << "ence of -d) or consolidation (-C) operations)\n\nThe initial sta";
  cout << "te is the source-state mentioned in the first such expression.\n";
  cout << "\"weight\" can be omitted, for a default weight of one.  \"outpu";
  cout << "t\" can also be\nommitted, in which case the output is the same ";
  cout << "as the input.  This program\noutputs transducers using the first";
  cout << " option, giving only one expression per\n\"source-state\", and e";
  cout << "xhaustively listing every arc out of that state in no\nparticula";
  cout << "r order, omitting \"weight\" and \"output\" whenever they are no";
  cout << "t\nneeded.  However, inputs may even contain many of these expre";
  cout << "ssions sharing\nthe same \"source-state\", in which case the arc";
  cout << "s are added to the existing\nstate.\n\nWhen the -n or -b options";
  cout << " are used, the input is instead sequences of\nspace-separated sy";
  cout << "mbols, from which a finite state automata accepting exactly\ntha";
  cout << "t sequence with weight one (a FSA is simply a transducer where v";
  cout << "ery arc has\nmatching input and output, and a weight of one).  W";
  cout << "ith the -b (batch) option,\nthe input may consist of any number ";
  cout << "of such sequences, each on their own\nline.  Each input sequence";
  cout << " must be no longer than 60000 characters.\n\nWhen the -m option ";
  cout << "is specified, meaningful names will be assigned to states\ncreat";
  cout << "ed in the composition of two WFST, in the following format (othe";
  cout << "rwise\nstate names are numbers assigned sequentially):\n\nlhs-WF";
  cout << "ST-state-name|filter-state|rhs-WFST-state-name\n\n\"filter-state";
  cout << "\" is a number between 0 and 2, representing the state in an\nim";
  cout << "plicit intermediate transducer used to eliminate redundant paths";
  cout << " using empty\n(*e*) transitions.\n\nWhen -k number-of-paths is u";
  cout << "sed without limiting the output with -I or -O,\npaths are displa";
  cout << "yed as a list of space-separated, parenthesized arcs:\n\n(input-";
  cout << "symbol : output-symbol / weight -> destination-state) \n\nfollow";
  cout << "ed by the path weight (the product of the weights in the sequenc";
  cout << "e) at\nthe end of the line.\n\n";
}
