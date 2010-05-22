#!/usr/bin/env python2.6
doc="""Minimal ghkm rule extraction w/ ambiguous attachment (unaligned f words) -> highest possible node.  Line numbers start at 0.  Headers start with ###.  Alignments are e-f 0-indexed.  Confusing characters in tokens are presumed to be removed already from input parses/strings; no escaping.

TODO: get actual #nonterms, sourcevocab from input etrees (currently hardcoded 40, 1000)
TODO: better base models
TODO: anneal?  avg counts not final?
TODO: output per-sentence alignment accuracy also?  to .info file?
"""

version="0.9"


import os,sys,itertools,re,operator,collections,random,math
from itertools import izip
sys.path.append(os.path.dirname(sys.argv[0]))

import pdb
import tree
import optparse

import unittest

from graehl import *
from dumpx import *


viz='viz-tree-string-pair.pl'

def rulefrag(t):
    return filter_children(t,lambda x:x.span is not None)

import string
parenescape=string.maketrans('()','[]')

less_checking=True

noisy_log=False
def log_change(type,olds,news,parnode,*rest):
    if noisy_log:
        sys.stderr.write(type+': old='+pstr(map(cstr,olds))+' new='+pstr(map(cstr,news))+' parnode(after)='+richs(parnode,names=False)+' '+pstr(rest)+'\n')

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
    if less_checking: return
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

    def update_vocabsize(self,nt,nnt):
        self.sourcevocab=nt
        self.nonterms=nnt
        self.update_model()

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
        self.group=group  #TODO: speedup: store ref to directly mutable cell rather than root label for hash
    def reset(self):
        self.count=0
#        self.count=self.prior
    def size(self):
        "return some notion of size (number of etree nodes?) of rule. for now just # of bytes"
        return len(self.rule)
    def __str__(self):
        return "{{{count=%d p0*a=%g %s}}}"%(self.count,self.prior,self.rule)

def cstr(n):
    return '' if n is None else ((n if not hasattr(n,"rule") else n.rule) if n is not None else '')

def cnstr(n):
    return span_str(n.span)+cstr(n.count)

def cnstrs(*ns):
    return map(cnstr,ns)

class Histogram(object):
    """dict counting occurrences of keys; will provide sorted list of (key,count) or output to gnuplot format"""
    def __init__(self):
        self.c=dict()
    def count(self,k,d=1):
        "by setting d to some arbitrary number, you can have any (x,y) to be graphed as a histogram or line"
        c=self.c
        if k in c:
            c[k]+=d
        else:
            c[k]=d
        assert(c[k]>=0)
        return c[k]
    def get_sorted(self):
        "return sorted list of (key,count)"
        return [(k,self.c[k]) for k in sorted(self.c.iterkeys())]
    def get_binned(self,binwidth=10):
        "assuming numeric keys, return sorted list ((min,max),count) grouped by [min,min+binwidth),[min+binwidth,min+binwidth*2) etc."
        assert False
    def text_gnuplot(self,binwidth=None,line=False):
        "returns a gnuplot program that uses bars if line is False, and bins if binwidth isn't None"
        assert binwidth is None

        assert False
    def __str__(self):
        return str(self.get_sorted())

class Counts(object):
    """track counts and sum of counts for groups on the fly.  TODO: integerize groups (root nonterminals)"""
    def get(self,rule,prior,group):
        "return Count object c for rule"
        if rule in self.rules:
            return self.rules[rule]
        assert(prior<=1)
        r=Count(rule,prior*self.alpha,group)
        if group not in self.norms: self.norms[group]=self.alpha
#        dump(rule,prior,self.alpha,group,str(r))
        self.rules[rule]=r
        return r
    def used_rules(self):
        "return only those Count objects whose count>0"
        return [x for x in self.rules.itervalues() if x.count>0]
    def freq_hist(self):
        "return histogram of rule counts"
        h=Histogram()
        for r in self.used_rules():
            h.count(r.count)
        return h
    def size_hist(self):
        "return histogram of rule sizes"
        h=Histogram()
        for r in self.used_rules():
            h.count(r.size())
        return h
    def logprob(self,c):
        return log_prob(self.prob(c))
    def prob(self,c):
        return 1. if c is None else (c.count+c.prior)/self.norms[c.group]
        #todo: include prior in count but protect from underflow: epsilon+1 - 1 == 0 (bad, can't get logprob)
    def logprobm1(self,c):
#        dump('probm1',cstr(c),self.probm1(c))
        return math.log(self.probm1(c))
    def probm1(self,c):
        "return prob given all the other events but this one (i.e. sub 1 from num and denom)"
        if c is None: return 1.
        assertge(self.norms[c.group],self.alpha)
        return (c.count+c.prior-1.)/(self.norms[c.group]-1.)
    def add(self,c,d):
        "denominator n[c.group] is created if necessary, including alpha (added once only), and d is added to c.count and n[c.group]"
        if c is None: return
        #todo: garbage collection for keys w/ 0 count
        g=c.group
        n=self.norms
#        dump('delta norm count',d,n[g],c,str(c))
        n[g]+=d
        c.count+=d
#        dump(str(c),n[g],d)
        assertcmp(n[g],'>',0)
        assertcmp(c.count,'>=',0)
    def __init__(self,basep=basep_default):
        self.rules={} # todo: make count object have reference to norm count cell instead of looking up in hash?
        self.norms={} # on init, include the alpha term already (doesn't need to include base model p0 * alpha since p0s sum to 1)
        self.basep=basep
        self.alpha=basep.alpha
        #TODO: efficient top-down non-random order for expand -> have outside to parent rule available already
    @staticmethod
    def rule_parent(node):
        return node.find_ancestor(lambda n:n.span is not None)
    @staticmethod
    def is_rule_leaf(node):
        return all((n.span is None for n in node.all_children()))
    @staticmethod
    def swap_spans(n1,n2):
        s1=n1.span
        n1.span=n2.span
        n2.span=s1
    def swap(self,n1,n2,ex):
        "a swap of spans (and counts) means that parent rule may change if one of the spans was None"
        parnode=Counts.rule_parent(n1)
        asserteq(parnode,Counts.rule_parent(n2),"swap not common rule parents",richs(n1),richs(n2))
        assert Counts.is_rule_leaf(n1),"swap not rule leaf: "+richs(n1)
        assert Counts.is_rule_leaf(n2),"swap not rule leaf: "+richs(n2)
        assert n1.closure_span is None
        assert n2.closure_span is None
#        dump("swap?",richs(n1),richs(n2))
        if n1.span is None and n2.span is None: return
        cold1=n1.count #TODO: use list for old/new count +1 -1?
        cold2=n2.count
        pold=parnode.count
        self.add(parnode.count,-1)
        oldp=self.logprob(parnode.count)
        self.add(cold1,-1)
        old1=self.logprob(cold1)
        self.add(cold2,-1)
        old2=self.logprob(cold2)
        oldlogp=oldp+old1+old2
#        dump("oldp,old1,old2",oldp,old1,old2)
        Counts.swap_spans(n1,n2)
        newpc=self.count_for_node(parnode,ex)
        new1=self.count_for_node(n1,ex)
        new2=self.count_for_node(n2,ex)
#        dump(newpc,cnstrs(parnode))
        lnp=self.logprob(newpc)
        self.add(newpc,1)
        ln1=self.logprob(new1)
        self.add(new1,1)
        newlogp=lnp+ln1+self.logprob(new2)
        #check math on exchangability: add,add,add,pm1,pm1,pm1 == p,add,p,add,p,add
        usenew=choosei_logps([oldlogp,newlogp])
        if usenew==0:
            Counts.swap_spans(n1,n2) #swap back to original
            self.add(parnode.count,1)
            self.add(cold1,1)
            self.add(cold2,1) #TODO: test how close an approximation it is to not change the counts before getting logprobm1
            self.add(newpc,-1)
            self.add(new1,-1)
        else:
            self.add(new2,1)
            parnode.count=newpc
            n1.count=new1
            n2.count=new2
            Translation.update_span(n1,n1.span,n2.span) # these were already swapped; fix up closure spans
            Translation.update_span(n2,n2.span,n1.span)
            log_change("swapped",[cold1,cold2,pold],[new1,new2,newpc],parnode)
        # above is a slowdown but properly handles the corner case where 2 (or 3) of the new rules are identical, or, more likely, have the same normgroup.  but in a large training set it would be nearly correct to ignore that.  also, could assume that none of the old rules = new and not have to adjust counts at all.  just use probm1.
    def count_for_node(self,node,ex):
        "return rule count ref for rule headed at node (without setting node.count).  None if node doesn't head a rule"
        if node.span is None: return None
        rule,base=ex.xrs_str(node,self.basep)
        c=self.get(rule,base,node.label)
#        dump(cstr(c))
        return c
    def expand(self,node,ex):
        """apply blunsom EXPAND operator (random choice) - give t a new span contained in (grandN)-parent rule (p.span), but not infringing on siblings' fspan.  also update closure_spans.  p is a path from p[0] to t; p[i] may all need their closure_span expanded if we set t.span"""
        checkt(node)
        f2e=ex.f2enode
        def align(a,b,to):
            for i in range(a,b):
                f2e[i]=to
        minspan=node.closure_span
        parnode=Counts.rule_parent(node)
#        dump(richs(node))
        checkt(parnode)
        if parnode is None:
            assert(node.parent is None)
            return # can't adjust top node anyway
        parspan=parnode.span
        assert (parspan is not None)
        oldspan=node.span
#        newlogps=[self.logprobm1(parnode.count)+self.logprobm1(node.count)]
# above commented out because it's too high prob in unlikely event of parent rule = self rule
        oldpc=parnode.count
        oldnc=node.count
        self.add(oldpc,-1)
        plp=self.logprob(oldpc)
        self.add(oldnc,-1)
        nlp=self.logprob(oldnc)
        newlogps=[nlp+plp]
        newspans=[(oldspan,node.count,parnode.count)] # parallel to newlogps;  (span,count,parcount)
        #same as init to empty and then consider_span(oldspan) but remember old rule struct
        assert(parnode is not node)
        def consider_span(span):
            node.span=span
            parc=self.count_for_node(parnode,ex)
            lp=self.logprob(parc)
            self.add(parc,1)
            newc=self.count_for_node(node,ex)
            newlogps.append(lp+self.logprob(newc))
            newspans.append((span,newc,parc))
            self.add(parc,-1)
        closure=node.closure_span
        imax=parspan[1]
        jmin=parspan[0]+1
        if oldspan is not None:
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
        newspani=choosei_logps(newlogps) # make sure to fix: set node.span and node.count accordingly
#        dump(newspani,newspans[newspani],node.count,parnode.count)
        newspan,node.count,parnode.count=newspans[newspani]

        self.add(parnode.count,1)
        self.add(node.count,1)
        node.span=newspan
        if (newspan!=oldspan):
            oldpars="old parnode="+richs(parnode,names=False)
            Translation.update_span(node,newspan,oldspan) # sets span, fixing .count is None <=> .span is None
            log_change("expanded",[oldnc,oldpc],[node.count,parnode.count],parnode,oldpars)
            checkclosure(node)
            checkclosure(parnode)
            #FIXME: this is supposed to be efficient and correct.  make sure that it is!
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
#        dump(richs(node))
#        dump(richs(parnode))
        checkt(parnode)

    def __str__(self):
        return "\n".join(rules.itervalues())

class Translation(object):
    def __init__(self,etree,estring,a,f,lineno=None):
        self.etree=etree
        self.estring=estring
        self.a=a
        self.f=f
        self.nf=len(f)
        self.ne=len(estring)
        self.netree=etree.size()
        self.lineno=lineno
        self.have_derivation=False
        assert(self.nf==a.nf)
        assert(self.ne==a.ne)
        self.set_spans(self.etree,self.a.spanadje())

    def visit_swaps(self,counts):
        self.visit_swaps_r(counts,self.etree,[])

    def visit_swaps_r(self,counts,node,pch):
        "append to pch all children with a .span who don't have any .span under them.  return True iff no .span in node-rooted subtree.  try swapping any 2 nodes that have a rule on them already.  for swapping a none with something, you can shrink one then grow other.  may want to include none/some swap but would have to complicate eligible pairs computation (parent no longer eligible after child none receives a rule)"
        if node.span is None:
            return all([self.visit_swaps_r(counts,c,pch) for c in node.children])
        ch=[]
        noch=all([self.visit_swaps_r(counts,c,ch) for c in node.children])
        if noch:
            pch.append(node)
#        dump("swap",richs(node),map(richs,ch))
        for c1,c2 in unordered_pairs(ch): #TODO: any benefit to randomizing order of pairs, or, changing subsequent pairs considered if a swap is made?
            counts.swap(c1,c2,self)
        return False

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

    def xrs_logprob(self,root,basemodel):
        return math.log(self.xrs_prob(root,basemodel))

    def xrs_prob(self,root,basemodel):
        """same as xrs_str(root,basemodel)[1]"""
        if root.span is None: return 1.
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
        assert(t.span is not None)
        Translation.full_align(t,fa,unmarked_span(t.espan),unmarked_span(t.span))
        return fa

    @staticmethod
    def full_align(t,fa,emarks,fmarks):
        "if t.frontier_node, set es[e]=1 and fs[f]=1 for e in t.espan and f in t.fspan.  add to fa.efpairs the full_alignment"
        for c in t.children:
            Translation.full_align(c,fa,emarks,fmarks)
        if t.span is not None:
            es=span_points_fresh(t.espan,emarks)
            fs=span_points_fresh(t.span,fmarks)
            fa.fully_connect(es,fs)

    def set_espans(self):
        if hasattr(self,"espan_done"): return
        self.espan_done=True
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
        """for node under t, set node.closure_span to the smallest span enclosing node.children.closure_span and node.children.span.  closure_span must already be computed for children.  closure_span may be smaller than span (and may be None)"""
        t.closure_span=reduce(lambda x,y:span_cover(x,y.span or y.closure_span),t.children,None)
        #reduce(lambda x,y:span_cover(x,y.fspan),t.children,None)
##        dump(richs(t),"closure",t.closure_span)

    def set_closure_spans(self):
        for n in self.etree.postorder():
            Translation.set_closure_span(n)


    @staticmethod
    def update_span(t,new,old):
        """set t.span=new and update closure_spans upward if necessary; allow fspan to become out of date; fspan is just closure_span if span is None else span
        FIXME: test
        """
        dbgme=False
        if dbgme: pdb.set_trace()
        old=old or t.closure_span
        t.span=new
        assert(span_in(t.closure_span,new) or new is None)
        if new is None:
            new=t.closure_span
        if old==new:
            return
        p=t.parent
        while True: # p is a node whose (closure) span has changed from old to new
            par=p.closure_span
            assert(new in [y.span or y.closure_span for y in p.children])
            if True:
                new=reduce(lambda x,y:span_cover(x,y.span or y.closure_span),p.children,None)
            else:
#            dump(new,old,par,richs(p))
                if par is not None:
                    if old is not None and ((par[0]==old[0] and (new is None or par[0]<new[0])) or (par[1]==old[1] and (new is None or par[1]>new[1]))): # either side was formerly propping up an end of par, but was shrunk
                        new=reduce(lambda x,y:span_cover(x,y.span or y.closure_span),p.children,None)
                    else: # we grew or held equal both ends
                        new=span_cover(par,new)
            old=p.closure_span
            if new==old: break #no change
#            dump("update closure",new,richs(p))
            p.closure_span=new
            checkclosure(p)
            if p.span is not None: break
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

    def cache_prob(self,counts):
        "return log10(prob) of current derivation correctly under cache model"
        rcs=[t.count for t in self.etree.preorder() if t.count is not None]
        for r in rcs: counts.add(r,-1)
        lp=0.
        for r in rcs:
            p=counts.prob(r)
            if (p<=0.):
                warn("underflow in cache prob %g, using log10(zeroprob)=%g\n%s"%(p,log_zero,callerstring(2)))
                l=log_zero
            else:
                l=math.log10(p)
            lp+=l
            counts.add(r,1)
        return lp

class Training(object):
    def __init__(self,parsef,alignf,ff,opts,basep=basep_default):
        self.output_files=dict()
        self.parsef=parsef
        self.alignf=alignf
        self.ff=ff
        self.basep=basep
        self.counts=Counts(basep)
        self.golda=None
        self.examples=list(self.reader())
        self.opts=opts
        if opts.golda:
            self.golda=[Alignment(aline,t.ne,t.nf) for aline,t in izip(open(opts.golda),self.examples)]

    def adjust_basep(self):
        "set self.basep.{sourcevocab,nonterms} based on actual counts in examples"
        terms=set()
        nonterms=set()
        for x in self.examples:
            for t in x.etree.preorder():
                (terms if t.is_terminal() else nonterms).add(t.label)
        dump(len(terms),len(nonterms),nonterms)
        self.basep.update_vocabsize(len(terms),len(nonterms))

    def __str__(self):
        return "[parallel training: e-parse=%s align=%s foreign=%s]"%(self.parsef,self.alignf,self.ff)
    def reader(self):
        for eline,aline,fline,lineno in izip(open(self.parsef),open(self.alignf),open(self.ff),itertools.count(0)):
                yield Translation.parse_sent(eline,aline,fline,lineno)
    #todo: randomize order of reader inputs in memory for interestingly different gibbs runs?
    def gibbs(self):
        self.gibbs_prep()
        for iter in range(0,self.opts.iter):
            self.gibbs_iter(iter)
        report_zeroprobs()

    def gibbs_prep(self):
        self.adjust_basep()
        opts=self.opts
        log("Using gibbs sampling starting from minimal ghkm.")
        counts=self.counts
        if opts.randomize:
            random.shuffle(examples)
        xs=self.examples
        self.nf=sum(x.nf for x in xs)
        self.netree=sum(x.netree for x in xs)
        self.ne=sum(x.ne for x in xs)
        for ex in self.examples:
            for (rule,p,root) in ex.all_rules(self.basep):
                c=counts.get(rule,p,root.label)
                root.count=c
                counts.add(c,1)
            ex.make_count_attr()
            ex.set_closure_spans()
            ex.set_f2enode()
            checkt(ex.etree)
        log("gibbs prepared for %d iterations over %d examples totaling %d foreign words, %d english tree nodes with %d leaves"%(opts.iter,len(self.examples),self.nf,self.netree,self.ne))
        self.write_histogram()
        report_zeroprobs()


    def gibbs_iter(self,iter):
        opts=self.opts
        lp=0.
        ei=0
        for ex in self.examples:
            ei+=1
            root=ex.etree
            nodes=list(root.preorder())[1:] #exclude top node
            if not opts.terminals:
                nodes=[x for x in nodes if not x.is_terminal()]
            if opts.randomize: random.shuffle(nodes)
            for n in nodes:
                self.counts.expand(n,ex)
            if opts.swap:
                ex.visit_swaps(self.counts)
            if opts.outputevery and iter % opts.outputevery == 0:
                self.output_ex(ex,"iter=%d"%iter)
            elp=ex.cache_prob(self.counts)
            if opts.verbose>=2:
                log("iter=%d example=%d log10(cache-prob)=%f"%(iter,ei,elp))
            lp+=elp
        if opts.histogram:
            self.write_histogram(iter)
        log("gibbs iter=%d log10(cache-prob)=%f "%(iter,lp)+self.alignment_report(iter))
        report_zeroprobs()

    def write_histogram(self,iter=None):
        header="minimal ghkm" if iter is None else "gibbs iter %d"%iter
        log("%s size histogram: %s"%(header,self.counts.size_hist()))
        log("%s rule frequency histogram: %s"%(header,self.counts.freq_hist()))

    def alignment_report(self,iter=None):
        "returns string describing aggregate alignment p/r/f.  TODO: don't compute full alignment multiple times if also printing it in output_ex?"
        if self.golda is None: return ""
        ret=""
        if False and iter is not None:
            ret+="iter=%d "%iter
        fas=[ex.full_alignment() for ex in self.examples]
        agrees=[a.agreement(g) for a,g in zip(fas,self.golda)]
        agree=reduce(componentwise,agrees)
#        agree=reduce(lambda z,(ex,gold):componentwise(z,ex.full_alignment().agreement(gold),sum),zip(self.examples,self.golda))
        p,r=pr_from_agreement(*agree)
#        dump(agree,p,r)
        fs=Alignment.fstr(p,r)
        ret+="alignment "+fs
        return ret

    def output_ex(self,t,header='',aof=None):
        opts=self.opts
        getalign=aof is not None or opts.header_full_align
        if getalign:
            fa=t.full_alignment()
        print
        if opts.header:
            print "###",header,t,("full-align={{{%s}}}"%(fa) if opts.header_full_align else '')
        if opts.rules:
            for (r,p,_),id in izip(t.all_rules(self.basep,opts.quote),itertools.count(0)):
                print r+(" ### baseprob=%g line=%d id=%d"%(p,t.lineno,id) if opts.features else "")
        if opts.derivation:
            print t.derivation_tree()
        if aof is not None:
            aof.write(str(fa)+'\n')

    def output(self):
        ao=self.opts.alignment_out
        aof=(ao and open_out(ao)) or None
        for t in self.examples:
            self.output_ex(t,'',aof)

    def output_file(self,suffix):
        if suffix not in self.output_files:
            self.output_files[suffix]=open_out_prefix(self.opts.outbase,suffix)
        return self.output_files[suffix]

    def ghkm(self):
        for t in self.examples:
            t.ghkm(self.opts.terminals)

    def main(self):
        global noisy_log
        noisy_log=self.opts.verbose>3
        self.ghkm()
        log("minimal ghkm "+self.alignment_report())
        if self.opts.iter>0:
            self.gibbs()
        self.output()

def gextract(opts):
    if opts.header:
        justnames=['terminals','quote','attr','derivation','inbase']
        print "### gextract %s minimal %s"%(version,attr_str(opts))
        #"terminals=%s quote=%s attr=%s derivation=%s inbase=%s"%(opts.terminals,opts.quote,opts.attr,opts.derivation,opts.inbase)
    inbase=opts.inbase
    train=Training(inbase+".e-parse",inbase+".a",inbase+".f",opts)
    train.main()


### tests:


class TestTranslation(unittest.TestCase):
    def setUp(self):
        dump("init test")
        inbase='mono0.1'
        golda='mono0.1.a-gold'
        verbose=2
        terminals=False
        quote=True
        features=True
        header=True
        derivation=True
        alignment_out=None
        header_full_align=False
        rules=True
        randomize=False
        iter=1
        test=False
        outputevery=10
        header_full_align=False
        swap=True
        self.opts=Locals()
        self.train=Training(inbase+".e-parse",inbase+".a",inbase+".f",self.opts)
        random.seed(12345)
        histogram=""
    def test_output(self):
        tr=self.train
        opts=self.opts
        examples=self.train.examples
        tr.ghkm()
        tr.gibbs_prep()
        for iter in range(0,opts.iter):
            tr.gibbs_iter(iter)
        tr.output()


### main:

import optfunc

@optfunc.arghelp('alignment_out','write new alignment (fully connecting words in rules) here')
def optfunc_gextract(inbase="astronauts",terminals=False,quote=True,features=True,header=True,derivation=False,alignment_out=None,header_full_align=False,rules=True,randomize=False,iter=2,test=False,outputevery=0,verbose=1,swap=True,golda="",histogram=False,outbase="-"):
    if test:
        sys.argv=sys.argv[0:1]
        unittest.main()
    else:
        gextract(Locals())

optfunc.main(optfunc_gextract)

def main():
    opts,_=usage.parse_args()

if False and __name__ == "__main__":
    errors=main()
    if errors: sys.exit(errors)

