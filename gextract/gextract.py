#!/usr/bin/env python2.6
doc="""Minimal ghkm rule extraction w/ ambiguous attachment (unaligned f words) -> highest possible node.  Line numbers start at 0.  Headers start with ###.  Alignments are e-f 0-indexed.  Confusing characters in tokens are presumed to be removed already from input parses/strings; no escaping.
"""

TODO="""
tropical semiring (a*b=>a+b,a+b=>a+log1p(exp(b-a)) where b>a) if needed (large rules p0 may underflow?)
"""

version="0.9"


import os,sys,itertools,re,operator,collections,random
sys.path.append(os.path.dirname(sys.argv[0]))

import pdb
import tree
import optparse

import unittest

from graehl import *
from dumpx import *

def rulefrag(t):
    return filter_children(t,lambda x:x.span is not None)

import string
parenescape=string.maketrans('()','[]')
def richstr(t):
    if type(t) is tuple and len(t)==2:
        return "[%s,%s]"%t
    return string.translate(str(t),parenescape)

def richt(t,attrs=['label','span'],namesinline=False):
    #lambda x:','.join(("%s=%s"%(n,getattr(x,n)) if namesinline else str(getattr(x,n)))
    def newl(x):
        return ','.join([richstr(getattr(x,n)) if hasattr(x,n) else '' for n in attrs])
    return t.relabel(newl)

def richs(t,attrs=['label','span','closure_span'],names=True,namesinline=False):
    s=''
    if names:
        s='['+(','.join(attrs))+']'
    return s+str(richt(t,attrs))

def dumpt(t,msg=None,attrs=['label','span','closure_span']):
    dumph(msg)
    dump(richs(t,attrs))

def checkt(t):
    for node in t.preorder():
        checkrule(node)
        checkclosure(node)

def checkrule(t):
    asserteq((t.span is None),(t.count is None),richs(t))

def checkclosure(t):
    clo=reduce(lambda x,y:span_cover(x,y.span or y.closure_span),t.children,None)
    asserteq(clo,t.closure_span,richs(t))
    if t.span is not None:
        assert span_in(clo,t.span),richs(t)

usage=optparse.OptionParser(epilog=doc,version="%prog "+version)
usage.add_option("-r","--inbase",dest="inbase",metavar="PREFIX",help="input lines from PREFIX.{e-parse,a,f}")
usage.add_option("-t","--terminals",action="store_true",dest="terminals",help="allow terminal (word without POS) rule root")
usage.add_option("--unquote",action="store_false",dest="quote",help="don't surround terminals with double quotes.  no escape convention yet.")
usage.add_option("-d","--derivation",action="store_true",dest="derivation",help="print derivation tree following rules (label 0 is first rule)")
usage.add_option("--nofeatures",action="store_false",dest="attr",help="print ### line=N id=N attributes on rules")
usage.add_option("--noheader",action="store_false",dest="header",help="suppress ### header lines (outputs for a line are still blank-line separated)")
usage.add_option("--alignment-out",dest="alignment_out",metavar="FILE",help="write new alignment (fully connecting words in rules) here")
usage.add_option("--header-full-align",action="store_true",dest="header_full_align",help="write full-align={{{...}}} attribute in header, same as --alignment-out")
usage.add_option("-i","--iter",dest="iter",help="number of gibbs sampling passes through data",type="int")
usage.add_option("--norules",action="store_false",dest="rules",help="do not print rules")
usage.add_option("--randomize",action="store_true",dest="randomize",help="shuffle input sentence order for gibbs")
usage.set_defaults(inbase="astronauts",terminals=False,quote=True,features=True,header=True,derivation=False,alignment_out=None,header_full_align=False,rules=True,randomize=False
#                   ,header_full_align=True,iter=1                  #debugging
                   )

def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)

def xrs_quote(s,quote):
    return '"'+s+'"' if quote else s

def xrs_var(i):
    return 'x'+str(i)

def xrs_var_lhs(i,node,quote):
    return xrs_var(i)+':'+(xrs_quote(node.label,quote) if node.is_terminal() else node.label)

class XrsBase(object):
    "p0: base model for generating any rule given a root nonterminal"
    alpha=pow(10,6)
    "p(rule)=[count(rule)+p0(rule)*alpha]/[alpha+sum_{rules r w/ same root NT}count(r)]"
    pexpand=.5
    "probability of expanding a nonterminal (preterminals always expand into terminal w/ p=1).  if no expansion, then variable."
    pchild=.5
    "geometric dist. on # of children; pchild is prob to add another"
    pterm=.5
    "geometric dist. on # of source terminals (rule rhs); pterm prob to add another"
    "variables are placed one at a time w/ uniform distr. over # of possible placements, i.e. # of ways for placing 2 vars amongst t terminals = (t+1)*(t+2).  for n vars, prod_"
    sourcevocab=1000
    "uniformly choose which terminal out of this many possibilities"
    nonterms=40
    "uniformly choose which nonterminal out of this many possiblities.  note that which are preterminals is deduced from tree structure; keeping the labels disjoint is the parser's responsbility"

    def __str__(self):
        return attr_str(self)
    #,['alpha','pexpand','pchild','pterm','sourcevocab','nonterms'])

    def __init__(self):
        self.update_model()

    def update_model(self):
        self.psourceword=self.pterm/self.sourcevocab
        self.pnonterm=1./self.nonterms
        self.pendchild=1.-self.pchild
        self.pendterm=1.-self.pterm

    @staticmethod
    def ways_vars(n_t,n_nt):
        if n_nt==0:
            return 1.
        return reduce(operator.mul,map(float,range(n_t+1,n_t+n_nt+1)))

    def p_rhs(self,n_t,n_nt):
        return self.pendterm*pow(self.psourceword,n_t)/XrsBase.ways_vars(n_t,n_nt)

basep_default=XrsBase()

class Count(object):
    "prior = p0*alpha.  count includes prior"
    def __init__(self,rule,prior,group,count=0):
        self.rule=rule
        self.prior=prior
        self.count=count
        self.group=group
    def reset(self):
        self.count=self.prior
    def __str__(self):
        return "{{{count=%d p0=%g %s}}}"%(self.rule,self.prior,self.count)

class Counts(object):
    """track counts and sum of counts for groups on the fly.  TODO: integerize groups (root nonterminals)"""
    def get(self,rule,prior,group):
        "return Count object c for rule"
        if rule in self.rules:
            return self.rules[rule]
        r=Count(rule,prior,group)
        self.rules[rule]=r
        return r
    def prob(self,c):
        return c.count/self.norms[c.group]
    def add(self,c,d):
        #todo: garbage collection for keys w/ 0 count
        g=c.group
        n=self.norms
        if g in n: n[g]+=d
        else: n[g]=d+self.alpha
        c.count+=d
    def __init__(self,basep=basep_default):
        self.rules={} # todo: make count object have reference to norm count cell instead of looking up in hash?
        self.norms={} # on init, include the alpha term already (doesn't need to include base model p0 * alpha since p0s sum to 1)
        self.basep=basep
        self.alpha=basep.alpha

        #TODO: efficient top-down non-random order for expand -> have outside to parent rule available already
    def update_count(self,n,ex):
        "tree node n which has a .span (is a rule root) gets its old rule count decreased by 1 and new rule count increased by 1"
        oldc=n.count
        if n.span is not None:
            newrule,newbasep=ex.xrs_str(n,self.basep)
            newc=self.get(newrule,newbasep,n.label)
            self.add(newc,1)
            n.count=newc
        else:
            n.count=None
        if oldc is not None:
            self.add(oldc,-1)
    def rulep(self,node):
        "return 1.0 if span is None, else rule prob of node (span=None <=> count=None)"
#        dump(node.span,node.count,richs(node))
        asserteq((node.span is None),(node.count is None),richs(node))
        return self.prob(node.count) if node.span is not None else 1.
    def expand(self,node,ex):
        """apply blunsom EXPAND operator (random choice) - give t a new span contained in (grandN)-parent rule (p.span), but not infringing on siblings' fspan.  also update closure_spans.  p is a path from p[0] to t; p[i] may all need their closure_span expanded if we set t.span"""
        checkt(node)
        f2e=ex.f2enode
        minspan=node.closure_span
        parnode=node.find_ancestor(lambda n:n.span is not None)
        dump(richs(node))
        checkt(parnode)
        if parnode is None:
            assert(node.parent is None)
            return # can't adjust top node anyway
        parspan=parnode.span
        assert (parspan is not None)
        oldspan=node.span
        newspans=[(self.rulep(parnode)*self.rulep(node),oldspan)]
        def consider_span(span):
            node.span=span
            parprob=ex.xrs_prob(parnode,self.basep)
            prob=1. if span is None else ex.xrs_prob(node,self.basep)
#            dump(span,richs(node),prob,parprob)
            newspans.append((prob*parprob,span))
        closure=node.closure_span
        imax=parspan[1]
        jmin=parspan[0]+1
        consider_span(None)
        if closure is not None:
            assert(span_in(closure,parspan))
            imax=closure[0]
            jmin=closure[1]
#        dump(parspan[0],imax,jmin,parspan[1])
        for i in range(parspan[0],imax):
            fi=f2e[i]
            if fi is parnode or fi is node: # otherwise a sibling of parnode covers f[i]
                for j in range(max(i+1,jmin),parspan[1]):
                    newsp=(i,j)
                    if newsp!=oldspan:
                        consider_span(newsp)
                    f=f2e[j]
                    if not (f is parnode or f is node):
                        break
        node.span=oldspan # recover from mutilated invariant in consider_span
        newspan=choosep(newspans) # make sure to fix: set node.span and node.count accordingly
        def align(a,b,to):
            for i in range(a,b):
                f2e[i]=to
        if (newspan==oldspan):
            node.span=oldspan # field was overwritten when considering
        else:
            if newspan is None:
                if oldspan is not None:
                    align(oldspan[0],oldspan[1],parnode)
            elif oldspan is None:
                align(newspan[0],newspan[1],node)
            else:
                if newspan[0]<oldspan[0]:
                    align(newspan[0],oldspan[0],node)
                elif newspan[0]>oldspan[0]:
                    align(oldspan[0],newspan[0],parnode)
                if newspan[1]>oldspan[1]:
                    align(oldspan[1],newspan[1],node)
                elif newspan[1]<oldspan[1]:
                    align(newspan[1],oldspan[1],parnode)
            Translation.update_span(node,newspan) # sets span, breaking .count is None <=> .span is None
            self.update_count(node,ex) # fixes above
            self.update_count(parnode,ex)
        dump(richs(node))
        dump(richs(parnode))
        checkt(node)

    def __str__(self):
        return "\n".join(rules.itervalues())

class Alignment(object):
    apair=re.compile(r'(\d+)-(\d+)')
    def __init__(self,aline,ne,nf):
        "aline is giza-format alignment: '0-0 0-1 ...' (e-f index with 0<=e<ne, 0<=f<nf)"
        def intpair(stringpair):
            return (int(stringpair[0]),int(stringpair[1]))
        self.efpairs=[intpair(Alignment.apair.match(a).group(1,2)) for a in aline.strip().split()] if aline else []
        self.ne=ne
        self.nf=nf
    def copy_blank(self):
        "return a blank alignment of same dimensions"
        return Alignment(None,self.ne,self.nf)
    def fully_connect(self,es,fs):
        "es and fs are lists of e and f indices, fully connect cross product"
        for e in es:
            for f in fs:
                self.efpairs.append((e,f))
    def adje(self):
        "for e word index, list of f words aligned to it"
        return adjlist(self.efpairs,self.ne)
    def adjf(self):
        "inverse of adje (indexed by f)"
        return adjlist([(f,e) for (e,f) in self.efpairs],self.nf)
    def spanadje(self):
        "minimal covering span (fa,fb) for e word index's aligned f words"
        return [span_cover_points(adj) for adj in self.adje()]
    def spanadjf(self):
        "inverse of spanadje"
        return [span_cover_points(adj) for adj in self.adjf()]
    def __str__(self):
        return " ".join(["%d-%d"%p for p in self.efpairs])

class Translation(object):
    def __init__(self,etree,estring,a,f,lineno=None):
        self.etree=etree
        self.estring=estring
        self.a=a
        self.f=f
        self.nf=len(f)
        self.ne=len(estring)
        self.lineno=lineno
        self.have_derivation=False
        assert(self.nf==a.nf)
        assert(self.ne==a.ne)
        self.set_spans(self.etree,self.a.spanadje())

    def set_spans(self,enode,fspane,epos=0):
        "epos is enode's yield's starting position in english yield; returns ending position.  sets treenode.fspan foreign (a,b)"
        if enode.is_terminal():
            enode.fspan=fspane[epos]
            return epos+1
        else:
            span=None
            for c in enode.children:
                epos=self.set_spans(c,fspane,epos)
                span=span_cover(span,c.fspan)
            enode.fspan=span
            return epos

    def find_frontier(self,enode,allow_epsilon_rhs=False,cspan=None):
        """set treenode.frontier_node iff (GHKM) span and cspan are nonoverlapping; cspan is union of spans of nodes that aren't
descendant or ancestor'.  cspan is a mutable array that efficiently tracks the current complement span by counting the number of
times each word is covered"""
        if cspan is None:
            cspan=[1]*self.nf
        if enode.fspan is None:
            spanr=[]
            fr=allow_epsilon_rhs
        else:
            spanr=range(enode.fspan[0],enode.fspan[1])
            fr=True
            for i in spanr:
                assert(cspan[i]>0)
                cspan[i]-=1
                if cspan[i]>0:
                    fr=False
        enode.span=enode.fspan if fr else None
        for c in enode.children:
            if c.fspan is not None:
                for i in range(c.fspan[0],c.fspan[1]):
                    cspan[i]+=1
        for c in enode.children:
            self.find_frontier(c,allow_epsilon_rhs,cspan)
        for c in enode.children:
            if c.fspan is not None:
                for i in range(c.fspan[0],c.fspan[1]):
                    cspan[i]-=1
        for i in spanr:
            cspan[i]+=1

    def ghkm(self,leaves_are_frontier=False,allow_epsilon_rhs=False):
        self.have_derivation=True
        self.etree.fspan=(0,self.nf)
        self.find_frontier(self.etree,allow_epsilon_rhs)
        if not leaves_are_frontier:
            for c in self.etree.frontier():
                c.span=None

    def xrs_str(self,root,basemodel,quote=False):
        """return (rule string,basemodel prob) pair for rule w/ root"""
        assert(root.span is not None)
        b,e=root.span
        frhs=self.f[b:e]
        asserteq(len(frhs),e-b)
        lhs,pl=Translation.xrs_lhs_str(root,frhs,b,basemodel,quote)
        rhs,pr=Translation.xrs_rhs_str(frhs,b,e,basemodel,quote)
        return (lhs+' -> '+rhs,pl*pr)

    @staticmethod
    def xrs_lhs_str(t,foreign,fbase,basemodel,quote=False):
        """return pair of xrs rule lhs string and base model prob, with the foreign words corresponding to the rhs span being replaced by the tuple (i,node)
where xi:node.label was a variable in the lhs string; only the first foreign word in the variable is replaced.  foreign initially is
foreign_whole_sentence[fbase:x], i.e. index 0 in foreign is at the first word in t.span.  the x in the 3rd slot of return tuple is just an implementation detail; ignore it."""
        s,p,x=Translation.xrs_lhs_str_r(t,foreign,fbase,basemodel,quote,0,t)
        return (s,p/basemodel.pnonterm)

    @staticmethod
    def xrs_lhs_str_r(t,foreign,fbase,basemodel,quote,xn,parent):
        "xn is variable index"
        if t.is_terminal():
            return (xrs_quote(t.label,quote),basemodel.psourceword,xn)
        s=t.label+'('
        p=basemodel.pnonterm
        preterm=t.is_preterminal()
        nc=len(t.children)
        if not preterm: # preterms labels must be distinct from non-preterms
            p*=pow(basemodel.pchild,nc-1)*basemodel.pendchild
        for c in t.children:
            if c.span is not None:
                l=c.span[0]
                fi=c.span[0]-fbase
                assert span_in(c.span,parent.span),richs(parent)
                assertindex(fi,foreign)
                foreign[fi]=(xn,c)
                s+=xrs_var_lhs(xn,c,quote)
                xn+=1
            else:
                sc,pc,xn=Translation.xrs_lhs_str_r(c,foreign,fbase,basemodel,quote,xn,parent)
                s+=sc
                p*=pc
            s+=' '
        return (s[:-1]+')',p,xn)

    #TODO: bugfix with allow_epsilon_rhs
    @staticmethod
    def xrs_rhs_str(frhs,b,ge,basemodel,quote=False):
        rhs=""
        gi=b
        p=basemodel.pendterm
        n_nt=0
        n_t=0
        while gi<ge:
            c=frhs[gi-b]
            if type(c) is tuple:
                rhs+=xrs_var(c[0])
                n_nt+=1
                newi=c[1].span[1]
                assertgt(newi,gi)
                gi=newi
            else:
                n_t+=1
                rhs+=xrs_quote(c,quote)
                gi+=1
            rhs+=' '
        return (rhs[:-1],basemodel.p_rhs(n_t,n_nt))

    def frontier(self):
        for c in self.etree.preorder():
            if c.span is not None:
                yield c

    @staticmethod
    def xrsfrontier_rec(root,r):
        for c in root.children:
            if c.span is not None:
                r.append(c)
            else:
                Translation.xrsfrontier_rec(c,r)
        return r

    @staticmethod
    def xrsfrontier(root):
        "list of all frontier nodes below root with no other frontier nodes between them and root."
        return Translation.xrsfrontier_rec(root,[])

    @staticmethod
    def xrs_deriv(t,rulei=None):
        if not rulei: rulei=[0]
        i=rulei[0]
        rulei[0]+=1
        #"%d%s"%(i,span_str(t.span))
        r=tree.Node(i,[Translation.xrs_deriv(c,rulei) for c in Translation.xrsfrontier(t)])
        return r

    def derivation_tree(self):
        "derivation tree with label = index into all_rules"
        return Translation.xrs_deriv(self.etree)


    @staticmethod
    def n_rhs(node):
        "return (#vars,#terminals) in subtree at node; vars have a .span and stop recursion"
        fs=node.fspan
        if node.span is not None:
            return (1,0) #var
        if fs is None:
            return (0,0) #no rhs
        nt=fs[1]-fs[0]
        nv=0
        for c in node.children:
            cv,ct=Translation.n_rhs(c)
            nt-=ct
            nv+=cv
        return (nv,nt)

    def xrs_prob(self,root,basemodel):
        """same as xrs_str(root,basemodel)[1]"""
        assert root.span is not None
        return self.xrs_str(root,basemodel)[1]
        #TODO: test - should be faster since we don't generate rule string
        lhs_prob=Translation.xrs_prob_lhs_r(root,basemodel)/basemodel.pnonterm
        n_t,n_nt=Translation.n_rhs(root)
        rhs_prob=basemodel.p_rhs(n_t,n_nt)
        return lhs_prob*rhs_prob

    @staticmethod
    def xrs_prob_lhs_r(t,basemodel):
        if t.is_terminal():
            return basemodel.psourceword
        preterm=t.is_preterminal()
        p=basemodel.pnonterm
        if not preterm: # preterms labels must be distinct from non-preterms
            p*=pow(basemodel.pchild,len(t.children)-1)*basemodel.pendchild
        for c in t.children:
            if c.span is None:
                p*=Translation.xrs_prob_lhs_r(c,basemodel)
        return p

    def all_rules(self,basemodel,quote=False):
        "list of all minimal rules: (rule,p0,root node of rule)"
        return [self.xrs_str(c,basemodel,quote)+(c,) for c in self.frontier()]

    @staticmethod
    def fetree(etree):
        return etree.relabel(lambda t:t.label+span_str(t.span))

    def __str__(self):
        return "line=%d e={{{%s}}} #e=%d #f=%d a={{{%s}}} f={{{%s}}}"%(self.lineno,Translation.fetree(self.etree),self.ne,self.nf,self.a," ".join(self.f))

    @staticmethod
    def parse_sent(eline,aline,fline,lineno):
        etree=raduparse(eline)
        e=etree.yield_labels()
        f=fline.strip().split()
        a=Alignment(aline,len(e),len(f))
        return Translation(etree,e,a,f,lineno)

    def full_alignment(self):
        """return Alignment object where each rule's lexical items are in full bipartite alignment.  for a minimal-rules derivation, this alignment will induce exactly the same derivation if provided as the .a input"""
        assert(self.have_derivation)
        fa=self.a.copy_blank()
        t=self.etree
        self.set_espans()
        Translation.full_align(t,fa,unmarked_span(t.espan),unmarked_span(t.fspan))
        return fa

    @staticmethod
    def full_align(t,fa,emarks,fmarks):
        "if t.frontier_node, set es[e]=1 and fs[f]=1 for e in t.espan and f in t.fspan.  add to fa.efpairs the full_alignment"
        for c in t.children:
            Translation.full_align(c,fa,emarks,fmarks)
        if t.frontier_node:
            es=span_points_fresh(t.espan,emarks)
            fs=span_points_fresh(t.fspan,fmarks)
            fa.fully_connect(es,fs)

    def set_espans(self):
        if hasattr(self,"set_espan"): return
        self.set_espan=True
        Translation.set_espan(self.etree,0)

    @staticmethod
    def set_espan(t,ebase):
        """set node.espan for all nodes in subtree t, espan=[ebase,b), and returns b.
        b=ebase+N where N is the number of leaves under t.  writes fe[i]=node ith word is aligned to (fe must be None init)"""
        if t.is_terminal():
            ep=ebase+1
            t.espan=(ebase,ep)
            return ep
        else:
            ep=ebase
            for c in t.children:
                ep=Translation.set_espan(c,ep)
            t.espan=(ebase,ep)
            return ep

    @staticmethod
    def set_closure_span(t):
        """for node under t, set node.closure_span to the smallest span enclosing node.children.closure_span and node.children.span.  closure_span must already be computed for children"""
        t.closure_span=reduce(lambda x,y:span_cover(x,y.span or y.closure_span),t.children,None)
        #reduce(lambda x,y:span_cover(x,y.fspan),t.children,None)
##        dump(richs(t),"closure",t.closure_span)

    def set_closure_spans(self):
        for n in self.etree.postorder():
            Translation.set_closure_span(n)


    @staticmethod
    def update_span(t,new):
        """set t.span=new and update closure_spans upward if necessary; allow fspan to become out of date; fspan is just closure_span if span is None else span
        FIXME: test
        """
        old=t.span or t.closure_span
        t.span=new
        if new is None:
            new=t.closure_span
        if old==new:
            return
        p=t.parent
        while True: # p is a node whose (closure) span has changed from old to new
            par=p.closure_span
            dump(new,old,par,richs(p))
            if par is not None:
                if old is not None and ((par[0]==old[0] and (new is None or par[0]<new[0])) or (par[1]==old[1] and (new is None or par[1]>new[1]))): # either side was formerly propping up an end of par, but was shrunk
                    new=reduce(lambda x,y:span_cover(x,y.span or y.closure_span),p.children,None)
                else: # we grew or held equal both ends
                    new=span_cover(par,new)
            old=p.closure_span
            if new==old: break #no change
            p.closure_span=new
            if p.span is not None: break
            checkclosure(p)
            p=p.parent
        checkclosure(t)

    @staticmethod
    def f2enode(t,fe):
        """recursive for all t in subtree: wherever t.span isn't empty, align all unaligned words @i in it so fe[i]=t.  fe[i]=None means unaligned (all words should be aligned to top node if nothing else, though)"""
        for c in t.children: Translation.f2enode(c,fe)
        for p in span_points(t.span):
            if fe[p] is None:
                fe[p]=t

    def set_f2enode(self):
        fe=[None for x in range(0,self.nf)]
        Translation.f2enode(self.etree,fe)
        self.f2enode=fe

    def make_count_attr(self):
        for t in self.etree.preorder():
            if not hasattr(t,"count"):
                assert(t.span is None)
                t.count=None

class Training(object):
    def __init__(self,parsef,alignf,ff,basep=basep_default):
        self.parsef=parsef
        self.alignf=alignf
        self.ff=ff
        self.basep=basep
        self.counts=Counts(basep)
    def __str__(self):
        return "[parallel training: e-parse=%s align=%s foreign=%s]"%(self.parsef,self.alignf,self.ff)
    def reader(self):
        for eline,aline,fline,lineno in itertools.izip(open(self.parsef),open(self.alignf),open(self.ff),itertools.count(0)):
                yield Translation.parse_sent(eline,aline,fline,lineno)
    #todo: randomize order of reader inputs in memory for interestingly different gibbs runs?
    def gibbs(self,opts,examples):
        self.gibbs_prep(opts,examples)
        for iter in range(0,opts.iter):
            self.gibbs_iter(iter,opts,examples)

    def gibbs_prep(self,opts,examples):
        log("Using gibbs sampling starting from minimal ghkm.")
        counts=self.counts
        if opts.randomize:
            random.shuffle(examples)
        for ex in examples:
            for (rule,p,root) in ex.all_rules(self.basep):
                c=counts.get(rule,p,root.label)
                root.count=c
                counts.add(c,1)
            ex.make_count_attr()
            ex.set_closure_spans()
            ex.set_f2enode()
            checkt(ex.etree)

    def gibbs_iter(self,iter,opts,examples):
        log("gibbs iter=%d"%iter)
        for ex in examples:
            root=ex.etree
            nodes=list(root.preorder())[1:] #exclude top node
            if not opts.terminals:
                nodes=[x for x in nodes if not x.is_terminal()]
            if opts.randomize: random.shuffle(nodes)
            for n in nodes:
                self.counts.expand(n,ex)
            if opts.outputevery and iter % opts.outputevery == 0:
                self.output_ex(ex,opts,"iter=%d"%iter)

    def output_ex(self,t,opts,header='',aof=None):
        getalign=aof is not None or opts.header_full_align
        if getalign:
            fa=t.full_alignment()
        print
        if opts.header:
            print "###",header,t,("full-align={{{%s}}}"%(fa) if opts.header_full_align else '')
        if opts.rules:
            for (r,p,_),id in itertools.izip(t.all_rules(self.basep,opts.quote),itertools.count(0)):
                print r+(" ### baseprob=%g line=%d id=%d"%(p,t.lineno,id) if opts.features else "")
        if opts.derivation:
            print t.derivation_tree()
        if aof is not None:
            aof.write(str(fa)+'\n')

    def output(self,opts,examples):
        ao=opts.alignment_out
        if ao:
            aof=open_out(ao)
        for t in examples:
            self.output_ex(t,opts,aof)

    def main(self,opts):
        examples=list(self.reader())
        for t in examples:
            t.ghkm(opts.terminals)
        if opts.iter>0:
            self.gibbs(opts,examples)
        self.output(opts,examples)

def gextract(opts):
    if opts.header:
        justnames=['terminals','quote','attr','derivation','inbase']
        print "### gextract %s minimal %s"%(version,attr_str(opts))
        #"terminals=%s quote=%s attr=%s derivation=%s inbase=%s"%(opts.terminals,opts.quote,opts.attr,opts.derivation,opts.inbase)
    inbase=opts.inbase
    train=Training(inbase+".e-parse",inbase+".a",inbase+".f")
    train.main(opts)


### tests:


class TestTranslation(unittest.TestCase):
    def setUp(self):
        dump("init test")
        inbase='astronauts'
        terminals=False
        quote=True
        features=True
        header=True
        derivation=True
        alignment_out=None
        header_full_align=False
        rules=True
        randomize=False
        iter=10
        test=False
        outputevery=1
        header_full_align=False
        self.train=Training(inbase+".e-parse",inbase+".a",inbase+".f")
        self.opts=Locals()
        random.seed(12345)
    def test_output(self):
        tr=self.train
        opts=self.opts
        examples=list(tr.reader())
        for t in examples:
            t.ghkm(False)
            dumpt(t.etree,"after ghkm")
        tr.gibbs_prep(opts,examples)
        for iter in range(0,opts.iter):
            tr.gibbs_iter(iter,opts,examples)
        tr.output(opts,examples)


### main:

import optfunc

@optfunc.arghelp('alignment_out','write new alignment (fully connecting words in rules) here')
def optfunc_gextract(inbase="astronauts",terminals=False,quote=True,features=True,header=True,derivation=False,alignment_out=None,header_full_align=False,rules=True,randomize=False,iter=0,test=True,outputevery=0):
    if test:
        unittest.main()
    else:
        gextract(Locals())

optfunc.main(optfunc_gextract)

def main():
    opts,_=usage.parse_args()

if False and __name__ == "__main__":
    errors=main()
    if errors: sys.exit(errors)

