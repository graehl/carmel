/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#include <iostream>
#include <fstream>
#include <cctype>
#include <string>
#include <ctime>
#include "fst.h"
#include "assert.h"

#ifdef _MSC_VER	   // Microsoft VISUAL C++
//#include <crtdbg.h>
//#define MEMDEBUG		// checks heap at every allocation ; slow
#endif

#define VERSION "2.00b"  ;

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

void usageHelp(void);
void WFSTformatHelp(void);

int
#ifdef _MSC_VER
__cdecl 
#endif
main(int argc, char *argv[]){
#ifdef _MSC_VER 
#ifdef MEMDEBUG
  int tmpFlag = CrtSetDbgFlag(CRTDBGREPORTFLAG);
  tmpFlag |= CRTDBGCHECKALWAYSDF;
  CrtSetDbgFlag(tmpFlag);
#endif
#endif
  if ( argc == 1 ) {
    usageHelp();
    return 0;
  }
  int i, j;
  bool flags[256];
  for ( i = 0 ; i < 256 ; ++i ) flags[i] = 0;
  char *pc, **parm = new char *[argc-1];
  int nParms = 0;
  int kPaths = 0;
  int thresh = 128;
  Weight converge = 1E-5;
  int convergeFlag = 0;
  Weight smoothFloor = 0;
  Weight prune = 0;
  int pruneFlag = 0;
  int floorFlag = 0;
  int nGenerate = 0;
  int maxTrainIter = 256;
  int minGenArcs = 0;
  int labelStart = 0;
  int labelFlag = 0;
  ostream *fstout = &cout;
  for ( i = 1 ; i < argc ; ++i ) {
    if ((pc=argv[i])[0] == '-' && pc[1] != '\0' && !convergeFlag && !floorFlag && !pruneFlag && !labelFlag)
      while ( *(++pc) ) {
	if ( *pc == 'k' )
	  kPaths = -1;
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
	else if ( *pc == 'g' )
	  nGenerate = -1;
	else if ( *pc == 'M' )
	  maxTrainIter = -1;
	else if ( *pc == 'L' )
	  minGenArcs = -1;
	else if ( *pc == 'N' )
	  labelFlag = 1;
	flags[*pc] = 1; 
      }
    else
      if ( labelFlag ) {
	labelFlag = 0;
	istringstream is(argv[i]);
	is >> labelStart; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -N switch, (instead got \'" << argv[i] << "\' - as a number, " << labelStart << ").\n";
	  return -11;
	}
      } else if ( kPaths == -1 ) {
	istringstream is(argv[i]);
	is >> kPaths; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -k switch, (instead got \'" << argv[i] << "\' - as a number, " << kPaths << ").\n";
	  return -11;
	}
	if ( kPaths < 1 )
	  kPaths = 1;
      } else if ( nGenerate == -1 ) {
	istringstream is(argv[i]);
	is >> nGenerate; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -g switch, (instead got \'" << argv[i] << "\' - as a number, " << nGenerate << ").\n";
	  return -11;
	}
	if ( nGenerate < 1 )
	  nGenerate = 1;
      } else if ( maxTrainIter == -1 ) {
	istringstream is(argv[i]);
	is >> maxTrainIter; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -M switch, (instead got \'" << argv[i] << "\' - as a number, " << maxTrainIter << ").\n";
	  return -11;
	}
	if ( maxTrainIter < 1 )
	  maxTrainIter = 1;
      } else if ( minGenArcs == -1 ) {
	istringstream is(argv[i]);
	is >> minGenArcs; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -M switch, (instead got \'" << argv[i] << "\' - as a number, " << minGenArcs << ").\n";
	  return -11;
	}
	if ( minGenArcs < 0 )
	  minGenArcs = 0;
      } else if ( thresh == -1 ) {
	istringstream is(argv[i]);
	is >> thresh; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -T switch, (instead got \'" << argv[i] << "\' - as a number, " << thresh << ").\n";
	  return -18;
	}
	if ( thresh < 0 )
	  thresh = 0;
      } else if ( convergeFlag ) {
	convergeFlag = 0;
	istringstream is(argv[i]);
	is >> converge; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -e switch, (instead got \'" << argv[i] << "\' - as a number, " << converge << ").\n";
	  return -30;
	}
      } else if ( floorFlag ) {
	floorFlag = 0;
	istringstream is(argv[i]);
	is >> smoothFloor; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -f switch, (instead got \'" << argv[i] << "\' - as a number, " << smoothFloor << ").\n";
	  return -30;
	}
      } else if ( pruneFlag ) {
	pruneFlag = 0;
	istringstream is(argv[i]);
	is >> prune; 
	if ( is.fail() ) {
	  cerr << "Expected a number after -p switch, (instead got \'" << argv[i] << "\' - as a number, " << prune << ").\n";
	  return -30;
	}
      } else if ( fstout == NULL ) {
	fstout = new ofstream(argv[i]);
	if ( !*fstout ) {
	  cerr << "Could not create file " << argv[i] << ".\n";
	  return -8;
	}
      } else
	parm[nParms++] = argv[i];
  }
  WFST::setIndexThreshold(thresh);
  if ( flags['h'] ) {
    WFSTformatHelp();
    return 0;
  }
  if (flags['V']){
    cerr << "Carmel Version: " << VERSION ;
    cerr << ". Copyright C 2000, the University of Southern California.\n";
    return 0 ;
  }
  if ( flags['b'] && kPaths < 1 ) 
    kPaths = 1;
  istream **inputs, **files;
  char **filenames;
  int nInputs;
  if ( flags['s'] ) {
    nInputs = nParms + 1;
    inputs = new istream *[nInputs];
    filenames = new char *[nInputs];
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
    nInputs = nParms;
    inputs = new istream *[nInputs];
    files = inputs;
    filenames = parm;
  }
  istream *pairStream = NULL;
  if ( flags['t'] )
    flags['S'] = 1;
  //(flags['S'] || 
  if ( nInputs < 1 || flags['A'] && nInputs < 2) {
    cerr << "No inputs supplied.\n";
    return -12;
  }
  for ( i = 0 ; i < nParms ; ++i ) {
//    if(parm[i][0]=='-' && parm[i][1] == '\0')
//		files[i] = &cin;
//	else
		files[i] = new ifstream(parm[i]);
    if ( !*files[i] ) {
      cerr << "File " << parm[i] << " could not be opened for input.\n";
      //for ( j = 0 ; j < i ; ++j )
		//delete files[i];
      return -9;
    }
  }
  if ( flags['S'] ) {
    flags['b'] = flags['x'] = flags['y'] = 0;
    kPaths = 0;
	if (nInputs > 1) {
	--nInputs;
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
  WFST *chainMemory = (WFST*)::operator new(nInputs * sizeof(WFST));
  WFST *chain = chainMemory;
  int nTarget = -1;
  if ( flags['i'] || flags['b']||flags['P']) // flags['P'] similar to 'i' but instead of simple transducer, produce permutation lattice.
    if ( flags['r'] )
      nTarget = nInputs-1;
    else
      nTarget = 0;
  
  for ( i = 0 ; i < nInputs ; ++i ) {
    if ( i != nTarget ) {
      new (&chain[i]) WFST(*inputs[i]);
      if ( !flags['m'] && nInputs > 1 )
	chain[i].unNameStates();
      if ( inputs[i] != &cin )
	delete inputs[i];
      if ( !(chain[i].valid()) ) {
	cerr << "Bad format of transducer file: " << filenames[i] << "\n";
	for ( j = i+1 ; j < nInputs ; ++j )
	  if ( inputs[i] != &cin )
	    delete inputs[i];
	return -2;
      }
    }
  }

  string buf ;
  
  WFST *result = NULL;
  WFST *weightSource = NULL;
  if ( flags['A'] ) {
    --nInputs;
    if ( !flags['r'] ) {
      weightSource = chain;
      ++chain;
    } else
      weightSource = chain + nInputs;
  }

  for ( ; ; ) {
      if (nTarget != -1) { // if to construct a finite state from input
      if ( !*inputs[nTarget] )
	break;
      *inputs[nTarget] >> ws;
      getline(*(inputs[nTarget]),buf);

      if ( !*inputs[nTarget] )
	break;
      if (flags['P']){ // need a permutation lattice instead
	int length ;	
	new (&chain[nTarget]) WFST(buf.c_str(),length,1);
      }
      
      else // no permutation, just need input acceptor
	new (&chain[nTarget]) WFST(buf.c_str());
#ifdef  DEBUGCOMPOSE
      cerr << "processing input " << buf.c_str() << '\n';
#endif      
      if ( !(chain[nTarget].valid()) ) {
	cerr << "Couldn't handle input line: " << buf << "\n";
	return -3;
      }
    }
    if ( flags['r'] ) {
      result = &chain[nInputs-1];
      if ( flags['C'] )
	result->consolidateArcs();
      if ( flags['p'] )
	result->prune(prune);
      if ( !flags['d'] )
	result->reduce();     
#ifdef  DEBUGCOMPOSE
      cerr << "result is chain[" << nTarget <<"]\n";      
#endif
      for ( i = nInputs-2 ; i >= 0 && result->valid() ; --i ) {	
#ifdef  DEBUGCOMPOSE
	cerr << "composing result with chain[" << i<<"] into next\n";
#endif	
	WFST *next = new WFST(chain[i], *result, flags['m'], flags['a']);
#ifndef NODELETE
	next->ownAlphabet(); 
	if (nTarget != -1 &&  (result !=  &chain[nTarget]) ){
#ifdef DEBUFCOMPOSE
	  cerr << "deleting result\n";	
#endif
	  delete result;
	}
#ifdef DEBUGCOMPOSE	
	cerr << "copying next into result\n";	
#endif
#endif
	result = next;
#ifdef  DEBUGCOMPOSE
	cerr <<"stats for the resulting composition\n";	
	cerr << "Number of states in result: " << result->count() << '\n';
	cerr << "Number of arcs in result: " << result->numArcs() << '\n';
	//      cerr << "Number of paths in result (without taking cycles): " << result->numNoCyclePaths() << '\n';
	//sleep(100);
	// cerr << "woke up\n";
#endif

	if ( !result->valid() ) {
	  cerr << "Empty or invalid result of composition with transducer " << filenames[i] << ".\n";
	  for ( i = 0 ; i < kPaths ; ++i ) {
	    if ( !flags['W'] )
	      cout << 0;
	    cout << "\n";	
	  }
	  goto nextInput;
	}
	if ( flags['C'] )
	  result->consolidateArcs();
	if ( flags['p'] )
	  result->prune(prune);
	if ( !flags['d'] )
	  result->reduce();
      }
#ifdef  DEBUGCOMPOSE
      cerr << "done chain of compositions  .. now processing result\n";
#endif
    }  // end of flag['r'] - right-to-left composition
    else { // left-to-right composition
      result = &chain[0];
      if ( flags['C'] )
	result->consolidateArcs();
      if ( flags['p'] )
	result->prune(prune);
      if ( !flags['d'] )
	result->reduce();
      for ( i = 1 ; i < nInputs && result->valid() ; ++i ) {
	WFST *next = new WFST(*result, chain[i], flags['m'], flags['a']);
#ifndef NODELETE
	next->ownAlphabet();      
	if ((nTarget != -1) &&  (result != &chain[nTarget]))
	  delete result;
#endif
	result = next;
	if ( !result->valid() ) {
	  cerr << "Empty or invalid result of composition with transducer " << filenames[i] << ".\n";
	  for ( i = 0 ; i < kPaths ; ++i ) {
	    if ( !flags['W'] )
	      cout << 0;
	    cout << "\n";	
	  }
	  goto nextInput;
	}
	if ( flags['C'] )
	  result->consolidateArcs();
	if ( flags['p'] )
	  result->prune(prune);
	if ( !flags['d'] )
	  result->reduce();
      } // end of chain compositions - now result points to the final composition
    } // end of left to right composition
    if ( flags['v'] )
      result->invert();
    if ( flags['n'] )
      result->normalizePerInput();
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
	for ( List<List<PathArc> >::const_iterator pli=bestPaths->begin()  ; pli != bestPaths->end(); ++pli, ++nGoodPaths ) {
	  Weight w = 1.0;
	  const char * outSym;
	  for (List<PathArc>::const_iterator li=pli->begin(); li != pli->end(); ++li ) {
	    if ( flags['O'] || flags['I'] ) {
	      if ( flags['O'] )
		outSym = li->out;
	      else
		outSym = li->in;
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
      cout << "Number of states in result: " << result->count() << '\n';
      cout << "Number of arcs in result: " << result->numArcs() << '\n';
      cout << "Number of paths in result (without taking cycles): " << result->numNoCyclePaths() << '\n';
    }
    if ( flags['t'] )
      flags['S'] = 0;
    if ( !flags['b'] ) {
      if ( flags['S'] ) {
		if (pairStream) {  
	for ( ; ; ) {
		
	  getline(*pairStream,buf);	  
	  if ( !*pairStream )
	    break;
	  List<int> *inSeq = result->symbolList(buf.c_str(), 0);
	  if ( !inSeq ) {
	    cerr << "Input sequence: " << buf << " contains symbols not in the alphabet.\n";
	    return -22;
	  }
	  getline(*pairStream,buf);
	  if ( !*pairStream )
	    break;
	  List<int> *outSeq = result->symbolList(buf.c_str(), 1);
	  if ( !outSeq ) {
	    cerr << "Output sequence: " << buf << " contains symbols not in the alphabet.\n";
	    return -21;
	  }
	  cout << result->sumOfAllPaths(*inSeq, *outSeq) << '\n';
	  delete inSeq;
	  delete outSeq;
	}
		} else {
			List<int> empty_list;
			cout << result->sumOfAllPaths(empty_list, empty_list) << '\n';
		}
      } else if ( flags['t'] ) {
	float weight;
	result->trainBegin();
	for ( ; ; ) {
	  weight = 1;
	  getline(*pairStream,buf);
	  if ( !*pairStream )
	    break;
	  if ( isdigit(buf[0]) || buf[0] == '-' || buf[0] == '.' ) {
	    istringstream w(buf.c_str());
	    w >> weight;
	    if ( w.fail() ) {
	      cerr << "Bad weight: " << buf << '\n';
	      continue;
	    }
	    getline(*pairStream,buf);
	    if ( !*pairStream )
	      break;
	  }
	  List<int> *inSeq = result->symbolList(buf.c_str(), 0);
	  if ( !inSeq ) {
	    cerr << "Input sequence: " << buf << " contains symbols not in the alphabet.\n";
	    return -22;
	  }
	  getline(*pairStream,buf);
	  if ( !*pairStream )
	    break;
	  List<int> *outSeq = result->symbolList(buf.c_str(), 1);
	  if ( !outSeq ) {
	    cerr << "Output sequence: " << buf << " contains symbols not in the alphabet.\n";
	    return -21;
	  }  
	  result->trainExample(*inSeq, *outSeq, weight);
	  delete inSeq;
	  delete outSeq;
	}
	result->trainFinish(converge, smoothFloor, maxTrainIter);
	if ( flags['p'] ) {
	  result->prune(prune);
	  result->normalizePerInput();
	}
	result->reduce();
      } else if ( nGenerate > 0 ) {
	if ( flags['d'] )
	  result->reduce();
	if ( !flags['n'] )
	  result->normalizePerInput();
	int maxSize = result->count() + minGenArcs;
	int *inSeq = new int[maxSize];
	int *outSeq = new int[maxSize];
	srand((unsigned)time(NULL));
	for ( int s = 0 ; s < nGenerate ; ++s ) {
	  while ( !result->generate(inSeq, outSeq, minGenArcs) ) ;
	  for ( i = 0 ; i < maxSize && inSeq[i] > 0 ; ) {
	    cout << result->inLetter(inSeq[i]);
	    if ( inSeq[++i] > 0 )
	      cout << ' ';
	  }
	  cout << '\n';
	  for ( i = 0 ; i < maxSize && outSeq[i] > 0 ; ) {
	    cout << result->outLetter(outSeq[i]);
	    if ( outSeq[++i] > 0 )
	      cout << ' ';
	  }
	  cout << '\n';
	}
	delete[] inSeq;
	delete[] outSeq;
      }
      if ( ( !flags['k'] && !flags['x'] && !flags['y'] && !flags['S']) && !flags['c'] && !flags['g'] || flags['F'] )
	*fstout << *result;
      break;
    }
  nextInput:
#ifndef NODELETE
    // Now delete the compostion result to free up memory, this is important 
    // when you are doing batch compostions (option -b).
    // But make sure  first that you are not deleting one of the main
    // WFSTs. That is why we check if the result is one of them and if it is
    // we don't delete it.
    bool isInChain = false ;    
    for (int i = 0 ; i < nInputs ; i++){
      if (result == &chain[i])
    	isInChain = true ;	
    }
    if (!isInChain){ 
#ifdef  DEBUGCOMPOSE
      cerr << "deleting result at end of processing\n";
#endif
      delete result ;
    }
#ifdef  DEBUGCOMPOSE
    else
      cerr << "can't delete result because it is part of chain .. \n";
#endif
    if (-1 != nTarget ){
#ifdef  DEBUGCOMPOSE
      cerr << "deleting the transducer constructed for this input: chain[" <<
	nTarget <<"]\n";
#endif
      chainMemory[nTarget].~WFST();
    }
#endif
    if ( !flags['b'] )
      break;
  } // end of all input
#ifndef NODELETE  
  for ( i = 0 ; i < nInputs ; ++i )
    if ( i != nTarget )
      chainMemory[i].~WFST();
  //  if ( flags['A'] )
  //    chainMemory[i].~WFST();
#endif
  ::operator delete(chainMemory);
  delete[] parm;
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
  cout << " is 1E-5 (or, -5log)\n-f w\t\tw is a floor weight used for train";
  cout << "ing, which is added to the\n\t\tcounts for all arcs, immediately";
  cout << " before normalization - if\n\t\tnonzero, it ensures that no arc ";
  cout << "will be given zero weight -\n\t\tdefault w is 0\n-M n\t\tn is th";
  cout << "e maximum number of training iterations that will be\n\t\tperfor";
  cout << "med, regardless of whether the convergence criteria is\n\t\tmet ";
  cout << "- default n is 256\n-x\t\tlist only the input alphabet of the tr";
  cout << "ansducer to stdout\n-y\t\tlist only the output alphabet of the t";
  cout << "ransducer to stdout\n-c\t\tlist only statistics on the transduce";
  cout << "r to stdout\n-F filename\twrite the final transducer to a file (";
  cout << "and not stdout), except \n\t\twhen -b is used\n-v\t\tinvert the ";
  cout << "resulting transducer by swapping the input and\n\t\toutput symbo";
  cout << "ls \n-d\t\tdo not eliminate dead-end states from all transducers";
  cout << " created\n-C\t\tconsolidate arcs with same source, destination, ";
  cout << "input and\n\t\toutput, with a total weight equal to the sum (cla";
  cout << "mped to a\n\t\tmaximum weight of one)\n-p w\t\tprune (discard) a";
  cout << "ll arcs with weight less than w\n-g n\t\tstochastically generate";
  cout << " n input/output pairs by following\n\t\trandom paths from the in";
  cout << "itial state to the final state\n\t\t(assumes that the weights ha";
  cout << "ve been normalized with -n or -t)\n\t\toutput is in the same for";
  cout << "m accepted in -t and -S\n-L n\t\twhile generating input/output p";
  cout << "airs, stop traveling when in the\n\t\tfinal state after at least";
  cout << " n arcs have been traversed, so that\n\t\tn <= (path length) <= ";
  cout << "(number of states) + n (default n is 0)\n-T n\t\tduring composit";
  cout << "ion, index arcs in a hash table when the\n\t\tproduct of the num";
  cout << "ber of arcs of two states is greater than n \n\t\t(by default, n";
  cout << " = 128)\n-N n\t\tassign each arc in the result transducer a uniq";
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
  cout << "-V\t\tversion number\n\n" ;  
  cout << "some formatting switches for displaying best paths:\n\t-I\tshow input symbols ";
  cout << "only\n\t-O\tshow output symbols only\n\t-E\tif -I or -O is speci";
  cout << "fied, omit special symbols (beginning and\n\t\tending with an as";
  cout << "terisk (e.g. \"*e*\"))\n\t-Q\tif -I or -O is specified, omit out";
  cout << "ermost quotes of symbol names\n\t-W\tdo not show weights for pat";
  cout << "hs\n\nconfused?  think you\'ve found a bug?  if all else fails, ";
  cout << "e-mail yaser@isi.edu or knight@isi.edu\n\n";
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
  cout << "sed without limiting the output with -i or -o,\npaths are displa";
  cout << "yed as a list of space-separated, parenthesized arcs:\n\n(input-";
  cout << "symbol : output-symbol / weight -> destination-state) \n\nfollow";
  cout << "ed by the path weight (the product of the weights in the sequenc";
  cout << "e) at\nthe end of the line.\n\n";
}
