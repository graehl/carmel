#include "train.h"
#include "fst.h"
#include "node.h"

void WFST::trainBegin(WFST::NormalizeMethod method) {
  consolidateArcs();
  normalize(method);
  delete trn;
  trn = new trainInfo;
  IOPair IO;
  DWPair DW;
  List<DWPair> *pLDW;
  HashTable<IOPair, List<DWPair> > *IOarcs =
    trn->forArcs = new HashTable<IOPair, List<DWPair> >[numStates()];
  HashTable<IOPair, List<DWPair> > *revIOarcs =
    trn->revArcs = new HashTable<IOPair, List<DWPair> >[numStates()];
  int s;
  for ( s = 0 ; s < numStates() ; ++s ){
    List<Arc>::iterator end = states[s].arcs.end();
    for ( List<Arc>::iterator aI=states[s].arcs.begin() ; aI != end; ++aI ) {
      IO.in = aI->in;
      IO.out = aI->out;
      int d = DW.dest = aI->dest;
      DW.arc = &(*aI);
      if ( !(pLDW = IOarcs[s].find(IO)) )
        pLDW = IOarcs[s].add(IO);
      pLDW->push(DW);
      DW.dest = s;
      if ( !(pLDW = revIOarcs[d].find(IO)) )
        pLDW = revIOarcs[d].add(IO);
      pLDW->push(DW);
    }
  }

  Graph eGraph = makeEGraph();
  trn->forETopo = topologicalSort(eGraph);
  Graph revEGraph = reverseGraph(eGraph);
  trn->revETopo = topologicalSort(revEGraph);
  delete[] revEGraph.states;
  delete[] eGraph.states;

  trn->f = trn->b = NULL;
  trn->maxIn = trn->maxOut = 0;
#ifdef DEBUGTRAIN
  std::cerr << "Just after training setup "<< *this ;
#endif
}

void WFST::trainExample(List<int> &inSeq, List<int> &outSeq, float weight)
{
  Assert(trn);
  Assert(weight > 0);
  IOSymSeq s;
  s.init(inSeq, outSeq, weight);
  //  trn->examples.push_back(s);
  trn->examples.insert(trn->examples.end(),s);
  if ( s.i.n > trn->maxIn )
    trn->maxIn = s.i.n;
  if ( s.o.n > trn->maxOut )
    trn->maxOut = s.o.n;
}

void WFST::trainFinish(Weight epsilon, Weight smoothFloor, int maxTrainIter,WFST::NormalizeMethod method)
{
  Assert(trn);
  int i, o, nSt = numStates();
  int maxIn = trn->maxIn;
  int maxOut = trn->maxOut;
  Weight ***f = trn->f = new Weight **[maxIn+1];
  Weight ***b = trn->b = new Weight **[maxIn+1];
  trn->nStates = nSt ; // Yaser - added for supporting a copy constrcutor for trainInfo this 7-26-2000
  for ( i = 0 ; i <= maxIn ; ++i ) {
    f[i] = new Weight *[maxOut+1];
    b[i] = new Weight *[maxOut+1];
    for ( o = 0 ; o <= maxOut ; ++o ) {
      f[i][o] = new Weight [nSt];
      b[i][o] = new Weight [nSt];
    }
  }

  trn->smoothFloor = smoothFloor;
  int giveUp = 0;
  Weight lastChange = 0;
  for ( ; ; ) {
    ++giveUp;
    if ( giveUp > maxTrainIter ) {
      std::cerr << "Maximum number of iterations (" << maxTrainIter << ") reached before convergence criteria of " << epsilon << " was met - last change was " << lastChange << "\n";
      break;
    }
	lastChange = train(giveUp,method);
    std::cerr << "Training iteration " << giveUp << ": largest change was " << lastChange << "\n";
	if ( lastChange <= epsilon ) {
      std::cerr << "Convergence criteria of " << epsilon << " was met after " << giveUp << " iterations.\n";
      break;
    }

  }

  delete[] trn->forArcs;
  delete[] trn->revArcs;
  delete trn->forETopo;
  delete trn->revETopo;
#ifdef N_E_REPS
#endif
  List<IOSymSeq>::iterator end = trn->examples.end() ;
  for ( List<IOSymSeq>::iterator seq=trn->examples.begin() ; seq !=end ; ++seq )
    seq->kill();

  for ( i = 0 ; i <= maxIn ; ++i ) {
    for ( o = 0 ; o <= maxOut ; ++o ) {
      delete[] f[i][o];
      delete[] b[i][o];
    }
    delete[] f[i];
    delete[] b[i];
  }
  delete[] f;
  delete[] b;
  delete trn;
  trn = NULL;
}

void sumPaths(int nSt, int start, Weight ***w, HashTable<IOPair, List<DWPair> > *IOarcs, List<int> * eTopo, int nIn, int *inLet, int nOut, int *outLet)
{
  int i, o, s;
#ifdef N_E_REPS
  Weight *wNew = new Weight [nSt];
  Weight *wOld = new Weight [nSt];
#endif
  for ( i = 0 ; i <= nIn ; ++i )
    for ( o = 0 ; o <= nOut ; ++o )
      for ( s = 0 ; s < nSt ; ++s )
        w[i][o][s] = 0;

  w[0][0][start] = 1;

  IOPair IO;
  List<DWPair> *pLDW;

  for ( i = 0 ; i <= nIn ; ++i )
    for ( o = 0 ; o <= nOut ; ++o ) {
#ifdef DEBUGFB
      std::cerr <<"("<<i<<","<<o<<")\n";
#endif
      IO.in = 0;
      IO.out = 0;
#ifdef N_E_REPS
      for ( s = 0 ; s < nSt; ++s )
        wNew[s] = w[i][o][s];
#endif
      List<int>::const_iterator end = eTopo->end();
      for ( List<int>::const_iterator topI=eTopo->begin() ; topI != end; ++topI ) {
        s = *topI;
        if ( (pLDW = IOarcs[s].find(IO)) ){
          List<DWPair>::iterator end2 = pLDW->end();
          for ( List<DWPair>::iterator iDW=pLDW->begin() ; iDW !=end2; ++iDW ){
#ifdef DEBUGFB
            std::cerr << "w["<<i<<"]["<<o<<"]["<<iDW->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight("<< *iDW<<") ="<< w[i][o][iDW->dest] <<" + " << w[i][o][s] <<" * "<< iDW->weight() <<" = "<< w[i][o][iDW->dest] <<" + " << w[i][o][s] * iDW->weight() <<" = ";
#endif
            w[i][o][iDW->dest] += w[i][o][s] * iDW->weight();
#ifdef DEBUGFB
            std::cerr << w[i][o][iDW->dest] << '\n';
#endif
          }
        }
      }
#ifdef N_E_REPS
      // caveat: this method is wrong, although it will converge as N_E_REPS -> inf assuming the null transitions are normalized per source state, it can converge higher than it should by counting the same paths multiple times.  thus, N_E_REPS is not enabled =D
      for ( int rep = 0 ; rep < N_E_REPS ; ++rep ) {
        for ( s = 0 ; s < nSt; ++s )
          wOld[s] = wNew[s];
        for ( s = 0 ; s < nSt; ++s )
          wNew[s] = w[i][o][s];
        for ( s = 0 ; s < nSt; ++s ) {
          if ( (pLDW = IOarcs[s].find(IO)) ){
            List<DWPair>::const_iterator end = pLDW->end();
            for ( List<DWPair>::const_iterator iDW=pLDW->begin() ; iDW != end; ++iDW )
              wNew[iDW->dest] += wOld[s] * iDW->weight();
          }
        }
      }
#endif
      for ( s = 0 ; s < nSt; ++s ) {
#ifdef N_E_REPS
        w[i][o][s] = wNew[s];
#endif
        if ( w[i][o][s] == 0 )
          continue;
        if ( o < nOut ) {
          IO.in = 0;
          IO.out = outLet[o];
          if ( (pLDW = IOarcs[s].find(IO)) ){
            List<DWPair>::iterator end = pLDW->end();
            for ( List<DWPair>::iterator iDW=pLDW->begin() ; iDW != end; ++iDW ){
#ifdef DEBUGFB
              std::cerr << "w["<<i<<"]["<<o+1<<"]["<<iDW->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight ("<< *iDW<<") ="<< w[i][o+1][iDW->dest] <<" + " << w[i][o][s] <<" * "<< iDW->weight() <<" = "<< w[i][o+1][iDW->dest] <<" + " << w[i][o][s] * iDW->weight() <<" = ";
#endif
              w[i][o+1][iDW->dest] += w[i][o][s] * iDW->weight();
#ifdef DEBUGFB
              std::cerr << w[i][o+1][iDW->dest] << '\n';
#endif
            }
          }
          if ( i < nIn ) {
            IO.in = inLet[i];
            IO.out = outLet[o];
            if ( (pLDW = IOarcs[s].find(IO)) ){
              List<DWPair>::iterator end = pLDW->end();
              for ( List<DWPair>::iterator iDW=pLDW->begin() ; iDW != end; ++iDW ){
#ifdef DEBUGFB
                std::cerr << "w["<<i+1<<"]["<<o+1<<"]["<<iDW->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight ("<< *iDW<<") ="<< w[i+1][o+1][iDW->dest] <<" + " << w[i][o][s] <<" * "<< iDW->weight() <<" = "<< w[i+1][o+1][iDW->dest] <<" + " << w[i][o][s] * iDW->weight() <<" = ";
#endif
                w[i+1][o+1][iDW->dest] += w[i][o][s] * iDW->weight();
#ifdef DEBUGFB
                std::cerr << w[i+1][o+1][iDW->dest] << '\n';
#endif
              }
            }
          }
        }
        if ( i < nIn ) {
          IO.in = inLet[i];
          IO.out = 0;
          if ( (pLDW = IOarcs[s].find(IO)) ){
            List<DWPair>::iterator end = pLDW->end();
            for ( List<DWPair>::iterator iDW=pLDW->begin() ; iDW !=end; ++iDW ){
#ifdef DEBUGFB
              std::cerr << "w["<<i+1<<"]["<<o<<"]["<<iDW->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight ("<< *iDW <<") ="<< w[i+1][o][iDW->dest] <<" + " << w[i][o][s] <<" * "<< iDW->weight() <<" = "<< w[i+1][o][iDW->dest] <<" + " << w[i][o][s] * iDW->weight() <<" = ";
#endif
              w[i+1][o][iDW->dest] += w[i][o][s] * iDW->weight();
#ifdef DEBUGFB
              std::cerr << w[i+1][o][iDW->dest] << '\n';
#endif
            }
          }
        }
      }
    }
#ifdef N_E_REPS
  delete[] wNew;
  delete[] wOld;
#endif
}

void forwardBackward(IOSymSeq &s, trainInfo *trn, int nSt, int final)
{
  Assert(trn);
#ifdef DEBUGFB
  std::cerr << "training example: \n"<<s << "\nForward\n" ;
#endif
  sumPaths(nSt, 0, trn->f, trn->forArcs, trn->forETopo, s.i.n, s.i.let, s.o.n, s.o.let);
#ifdef DEBUGFB
  std::cerr << "\nBackward\n";
#endif
  sumPaths(nSt, final, trn->b, trn->revArcs, trn->revETopo, s.i.n, s.i.rLet, s.o.n, s.o.rLet);
  int i;
  Weight ***w = trn->b;
  int nIn = s.i.n;
  int nOut = s.o.n;
  for ( i = 0 ; i <= nIn/2 ; ++i ) {
    Weight **temp = w[i];
    w[i] = w[nIn - i];
    w[nIn - i] = temp;
  }
  for ( i = 0 ; i <= nIn ; ++i )
    for ( int o = 0 ; o <= nOut/2 ; ++o ) {
      Weight *temp = w[i][o];
      w[i][o] = w[i][nOut - o];
      w[i][nOut - o] = temp;
    }
#ifdef DEBUGTRAIN // Yaser 7-20-2000
  std::cerr << "\nForwardProb/BackwardProb:\n";
  for (i = 0 ;i<= nIn ; ++i){
    for (int o = 0 ; o <= nOut ; ++o){
      std::cerr << '(' ;
      for (int s = 0 ; s < nSt ; ++s){
        std::cerr << trn->f[i][o][s] <<'/'<<trn->b[i][o][s];
        if (s < nSt-1)
          std::cerr << ' ' ;
      }
      std::cerr <<')';
      if(o < nOut-1)
        std::cerr <<' ' ;
    }
    std::cerr <<'\n';
  }
#endif
}

Weight WFST::train(const int iter,WFST::NormalizeMethod method)
{
  Assert(trn);
  int i, o, s, nIn, nOut, *letIn, *letOut;
  Weight ***f = trn->f;
  Weight ***b = trn->b;
  HashTable <IOPair, List<DWPair> > *IOarcs;
  List<DWPair> * pLDW;
  IOPair io;
#ifdef DEBUGTRAIN
  std::cerr << "Starting iteration: " << iter << '\n';
#endif
  for ( s = 0 ; s < numStates() ; ++s )
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
      List<DWPair>::iterator end = ha.val().end() ;
      for ( List<DWPair>::iterator dw=ha.val().begin() ; dw !=end ; ++dw )
        dw->counts = 0;
    }

  List<IOSymSeq>::iterator seq=trn->examples.begin() ;
  List<IOSymSeq>::iterator lastExample=trn->examples.end() ;

#ifdef DEBUGTRAIN
  int train_example_no = 0 ; // Yaser 7-13-2000
#endif
  while (seq != lastExample) { // loop over all training examples

#ifdef DEBUGTRAIN // Yaser 13-7-2000 - Debugging messages ..
    ++train_example_no ;
    std::cerr << '\n';
    if (train_example_no % 100 == 0)
      std::cerr << '.' ;
    if (train_example_no % 7000 == 0)
      std::cerr << train_example_no << '\n' ;
#endif
    nIn = seq->i.n;
    nOut = seq->o.n;
    forwardBackward(*seq, trn, numStates(), final);
#ifdef DEBUGTRAIN
    std::cerr << '\n';
    for ( i = 0 ; i < nIn ; ++i ) {
      std::cerr << (*in)[seq->i.let[i]] << ' ' ;
      for ( o = 0 ; o < nOut ; ++o ) {
        std::cerr << (*out)[seq->o.let[o]] << ' ' ;
        std::cerr << '(' << f[i][o][0];
        for ( s = 1 ; s < numStates() ; ++s ) {
          std::cerr << ' ' << f[i][o][s];
        }
        std::cerr << ") ";
      }
      std::cerr << "\t\t";
      for ( o = 0 ; o < nOut ; ++o ) {
        std::cerr << (*out)[seq->o.let[o]] << ' ' ;
        std::cerr << '(' << b[i][o][0];
        for ( s = 1 ; s < numStates() ; ++s ) {
          std::cerr << ' ' << b[i][o][s];
        }
        std::cerr << ") ";
      }
      std::cerr << '\n';
    }
#endif
    Weight fin = f[nIn][nOut][final];
    if ( fin == 0 ) {
      std::cerr << "No accepting path in transducer for input/output:\n";
      for ( i = 0 ; i < nIn ; ++i )
        std::cerr << (*in)[seq->i.let[i]] << ' ';
      std::cerr << '\n';
      for ( o = 0 ; o < nOut ; ++o )
        std::cerr << (*out)[seq->o.let[o]] << ' ';
      std::cerr << '\n';
      trn->examples.erase(seq++);
      continue;
    }
#ifdef ALLOWED_FORWARD_OVER_BACKWARD_EPSILON
    Weight fin2 = b[0][0][0];
    double ratio = (fin/fin2).getReal();
    double e = ratio - 1;
    if ( e < 0 )
      e = -e;
    if ( e > ALLOWED_FORWARD_OVER_BACKWARD_EPSILON )
      std::cerr << "Roundoff error of " << e << " exceeded " << ALLOWED_FORWARD_OVER_BACKWARD_EPSILON << ".\n";
#endif

    letIn = seq->i.let;
    letOut = seq->o.let;
    // initialize scratch counts to zero
    for ( s = 0 ; s < numStates() ; ++s )
      for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
        List<DWPair>::iterator end = ha.val().end() ;
        for ( List<DWPair>::iterator dw=ha.val().begin() ; dw !=end ; ++dw )
          dw->scratch = 0;
      }

    for ( i = 0 ; i <= nIn ; ++i ) // go over all symbols in input in the training pair
      for ( o = 0 ; o <= nOut ; ++o ) // go over all symbols in the output pair
        for ( s = 0 ; s < numStates() ; ++s ) {
          IOarcs = trn->forArcs + s;
          if ( i < nIn ) { // input is not epsilon
            io.in = letIn[i];
            if ( o < nOut ) { // output is not epsilon
              io.out = letOut[o];
              if ( (pLDW = IOarcs->find(io)) ){
                List<DWPair>::iterator end =  pLDW->end();
                for ( List<DWPair>::iterator dw=pLDW->begin() ; dw !=end; ++dw )
                  dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o+1][dw->dest];
              }
            }
            io.out = 0; // output only is epsilon
            if ( (pLDW = IOarcs->find(io)) ){
              List<DWPair>::iterator end = pLDW->end()  ;
              for ( List<DWPair>::iterator dw= pLDW->begin() ; dw !=end; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o][dw->dest];
            }
          }
          io.in = 0; // input is epsilon
          if ( o < nOut ) { // input only is epsilon
            io.out = letOut[o];
            if ( (pLDW = IOarcs->find(io)) ){
              List<DWPair>::iterator end = pLDW->end()  ;
              for ( List<DWPair>::iterator dw=pLDW->begin() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i][o+1][dw->dest];
            }
          }
          io.out = 0; // input and output are epsilon
          if ( (pLDW = IOarcs->find(io)) ){
            List<DWPair>::iterator end = pLDW->end() ;
            for ( List<DWPair>::iterator dw=pLDW->begin() ; dw !=end ; ++dw )
              dw->scratch += f[i][o][s] * dw->weight() * b[i][o][dw->dest];
          }
        }
    for ( s = 0 ; s < numStates() ; ++s )
      for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
        List<DWPair>::iterator end = ha.val().end();
        for ( List<DWPair>::iterator dw=ha.val().begin() ; dw !=end; ++dw ) {
          dw->counts += (dw->scratch / fin) * (Weight)seq->weight;
        }
      }
    ++seq;
  } // end of while(seq)
  int pGroup;
#ifdef DEBUGTRAINDETAIL
  std::cerr << "\nWeights before tied groups\n";
  for ( s = 0 ; s < numStates() ; ++s )
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
      List<DWPair>::const_iterator end = ha.val().end() ;
      for ( List<DWPair>::const_iterator dw=ha.val().begin() ; dw !=end; ++dw ){
        if ((pGroup = (dw->arc)->groupId) >= 0)
          std::cerr << pGroup << ' ' ;
        std::cerr << s << "->" << *dw->arc <<  " weight " << dw->weight() << " scratch: "<< dw->scratch  <<" counts " <<dw->counts  << '\n';
      }
    }

#endif
  for ( s = 0 ; s < numStates() ; ++s )
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
      List<DWPair>::iterator end = ha.val().end();
      for ( List<DWPair>::iterator dw=ha.val().begin() ; dw !=end; ++dw )
        if ((pGroup = (dw->arc)->groupId) != 0) { // if the group is tied, and the group number is zero, then the old weight doe not change. Otherwise update as follows
#ifdef DEBUGTRAINDETAIL
          std::cerr << "Arc " <<*dw->arc <<  " in tied group " << pGroup <<'\n';
#endif
          dw->scratch = dw->weight();   // old weight - Yaser: this is needed only to calculate change in weight later on ..
          dw->weight() = dw->counts + trn->smoothFloor; // new (unnormalized weight)
        }
    }
#ifdef DEBUGTRAINDETAIL
  std::cerr << "\nWeights before normalization\n";
  for ( s = 0 ; s < numStates() ; ++s )
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
      List<DWPair>::const_iterator end = ha.val().end() ;
      for ( List<DWPair>::const_iterator dw=ha.val().begin() ; dw !=end; ++dw ){
        if ((pGroup = (dw->arc)->groupId) >= 0)
          std::cerr << pGroup << ' ' ;
        std::cerr << s << "->" << *dw->arc <<  " weight " << dw->weight() << " scratch: "<< dw->scratch  <<" counts " <<dw->counts  << '\n';
      }
    }
#endif
  normalize(method);
#ifdef DEBUGTRAINDETAIL
  std::cerr << "\nWeights after normalization\n";
  for ( s = 0 ; s < numStates() ; ++s )
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
      List<DWPair>::const_iterator end = ha.val().end() ;
      for ( List<DWPair>::const_iterator dw=ha.val().begin() ; dw !=end; ++dw )
        std::cerr << s << "->" << *dw->arc <<  " weight " << dw->weight() << " scratch: "<< dw->scratch  <<" counts " <<dw->counts  << '\n';
    }

#endif
  // find maximum change for convergence
  Weight change, maxChange = 0;
  for ( s = 0 ; s < numStates() ; ++s )
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){
      List<DWPair>::iterator end = ha.val().end();
      for ( List<DWPair>::iterator dw=ha.val().begin() ; dw != end; ++dw )
        if ((pGroup = (dw->arc)->groupId)!=0 ) {
          if ( dw->scratch > dw->weight() )
            change = dw->scratch - dw->weight();
          else
            change = dw->weight() - dw->scratch;
          if ( change > maxChange )
            maxChange = change;
        }
    }
  return maxChange;
}

Weight ***WFST::forwardSumPaths(List<int> &inSeq, List<int> &outSeq)
{
  int i, o, s;
  int nIn = inSeq.length();
  int nOut = outSeq.length();
  int *inLet = new int[nIn];
  int *outLet = new int[nOut];
  int *pi;
  pi = inLet;
  List<int>::const_iterator end = inSeq.end();
  for ( List<int>::const_iterator inL=inSeq.begin() ; inL != end; ++inL )
    *pi++ = *inL;
  pi = outLet;
  end = outSeq.end();
  for ( List<int>::const_iterator outL=outSeq.begin() ; outL != end; ++outL )
    *pi++ = *outL;

  HashTable<IOPair, List<DWPair> > *IOarcs =
    new HashTable<IOPair, List<DWPair> >[numStates()];

  IOPair IO;
  DWPair DW;
  List<DWPair> *pLDW;

  for ( s = 0 ; s < numStates() ; ++s ){
    List<Arc>::iterator end = states[s].arcs.end();
    for ( List<Arc>::iterator aI=states[s].arcs.begin() ; aI!=end; ++aI ) {
      IO.in = aI->in;
      IO.out = aI->out;
      DW.dest = aI->dest;
      DW.arc = &(*aI);
      if ( !(pLDW = IOarcs[s].find(IO)) )
        pLDW = IOarcs[s].add(IO);
      pLDW->push(DW);
    }
  }
  Graph eGraph = makeEGraph();
  List<int> * eTopo = topologicalSort(eGraph);
  delete[] eGraph.states;

  Weight ***w = new Weight **[nIn+1];
  for ( i = 0 ; i <= nIn ; ++i ) {
    w[i] = new Weight *[nOut+1];
    for ( o = 0 ; o <= nOut ; ++o )
      w[i][o] = new Weight [numStates()];
  }
  sumPaths(numStates(), 0, w, IOarcs, eTopo, nIn, inLet, nOut, outLet);

  delete eTopo;
  delete[] IOarcs;
  delete[] inLet;
  delete[] outLet;
  return w;
}


ostream & operator << (ostream &out, const trainInfo &t){ // Yaser 7-20-2000

  out << "Forward Edges Topologically Sorted\n" ;
  if (t.forETopo)
    out << *(t.forETopo)<<'\n';
  else out << "not set yet!";
  return(out);
}



ostream& operator << (ostream &out, struct State &s){ // Yaser 7-20-2000
  out << s.arcs << '\n';
  return(out);
}

ostream & operator << (ostream &o, IOPair p)
{
  return o << '(' << p.in << ' ' << p.out << ')';
}

int operator == (IOPair l, IOPair r) { return l.in == r.in && l.out == r.out; }

ostream & operator << (ostream &o, DWPair p)
{
  return o << *(p.arc)  ;
}

const int Entry<IOPair,List<DWPair> >::newBlocksize = 64;
Entry<IOPair,List<DWPair> > *Entry<IOPair,List<DWPair> >::freeList = NULL;

Node<DWPair> *Node<DWPair>::freeList = NULL;
const int Node<DWPair>::newBlocksize = 64;

ostream & hashPrint(HashTable<IOPair, List<DWPair> > &h, ostream &o) {
  HashIter<IOPair,List<DWPair> > i(h);
  if ( !i ) return o;
  o << '(' << i.key() << ' ' << i.val() << ')';
  while ( ++i )
    o << " (" << i.key() << ' ' << i.val() << ')';
  return o;
}


ostream & operator << (ostream & out , const symSeq & s){   // Yaser 7-21-2000
  for (int i=0 ; i < s.n ; ++i)
    out << s.let[i] ;
  out << '\n';
  return(out);
}

ostream & operator << (ostream & out , const IOSymSeq & s){   // Yaser 7-21-2000
  out << s.i << s.o ;
  return(out);
}

Node<IOSymSeq> *Node<IOSymSeq>::freeList = NULL;
const int Node<IOSymSeq>::newBlocksize = 64;

