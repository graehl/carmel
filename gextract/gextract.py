#!/usr/bin/env python2.6
doc="""Minimal ghkm rule extraction w/ ambiguous attachment (unaligned f words) -> highest possible node.  Line numbers start at 0.  Headers start with ###.  Alignments are e-f 0-indexed.  Confusing characters in tokens are presumed to be removed already from input parses/strings; no escaping.
"""
version="0.9"

import os,sys,itertools,re
sys.path.append(os.path.dirname(sys.argv[0]))

import tree
import optparse

usage=optparse.OptionParser(epilog=doc,version="%prog "+version)
usage.add_option("-r","--inbase",dest="inbase",metavar="PREFIX",help="input lines from PREFIX.{e-parse,a,f}")
usage.add_option("-t","--terminals",action="store_true",dest="terminals",help="allow terminal (word without POS) rule root")
usage.add_option("--unquote",action="store_false",dest="quote",help="don't surround terminals with double quotes.  no escape convention yet.")
usage.add_option("-d","--derivation",action="store_true",dest="derivation",help="print derivation tree following rules (label 0 is first rule)")
usage.add_option("--attr",action="store_true",dest="attr",help="print ### line=N id=N attributes on rules")
usage.add_option("--no-header",action="store_false",dest="header",help="suppress ### header lines (outputs for a line are still blank-line separated)")
usage.set_defaults(inbase="astronauts",terminals=False,quote=True,attr=False,header=True,derivation=True)

def intpair(stringpair):
    return (int(stringpair[0]),int(stringpair[1]))

def xrs_quote(s,quote):
    return '"'+s+'"' if quote else s

def xrs_var(i):
    return 'x'+str(i)

def xrs_var_lhs(i,node,quote):
    return xrs_var(i)+':'+(xrs_quote(node.label,quote) if node.is_terminal() else node.label)

radu_drophead=re.compile(r'\(([^~]+)~(\d+)~(\d+)\s+(-?[.0123456789]+)')
radu_lrb=re.compile(r'\((-LRB-(-\d+)?) \(\)')
radu_rrb=re.compile(r'\((-RRB-(-\d+)?) \)\)')
def radu2ptb(t):
    t=radu_drophead.sub(r'(\1',t)
    t=radu_lrb.sub(r'(\1 -LRB-)',t)
    t=radu_rrb.sub(r'(\1 -RRB-)',t)
    return t

def raduparse(t):
    t=radu2ptb(t)
    return tree.str_to_tree(t)

def adjlist(pairs,na):
    "return adjacency list indexed by [a]=[x,...,z] for pairs (a,x) ... (a,z)"
    adj=[[] for row in xrange(na)]
    for a,b in pairs:
        adj[a].append(b)
    return adj

def span_cover_points(points):
    "returns (a,b) for half-open [a,b) covering points"
    if (len(points)==0):
        return (0,0)
    return (min(points),max(points)+1)

def span_empty(s):
    return s[0]>=s[1]

def span_cover(sa,sb):
    """return smallest span covering both sa and sb; if either is 0-length then the other is returned
    (i.e. 0-length spans are empty sets, not singleton point sets)"""
    if sa is None:
        return sb
    if (sa[0]<sa[1]):
        if (sb[0]<sb[1]):
            return (min(sa[0],sb[0]),max(sa[1],sb[1])) # 0-length spans would confuse this formula
        return sa
    else:
        return sb

def span_str(s):
    if s is None:
        return ""
    return "[%d,%d]"%s

class Alignment(object):
    apair=re.compile(r'(\d+)-(\d+)')
    def __init__(self,aline,ne,nf):
        self.efpairs=[intpair(Alignment.apair.match(a).group(1,2)) for a in aline.strip().split()]
        self.ne=ne
        self.nf=nf
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
        assert(self.nf==a.nf)
        assert(self.ne==a.ne)
        self.set_spans(self.etree,self.a.spanadje())

    def set_spans(self,enode,fspane,epos=0):
        "epos is enode's yield's starting position in english yield; returns ending position.  sets treenode.span foreign (a,b)"
        if enode.is_terminal():
            enode.span=fspane[epos]
            return epos+1
        else:
            span=None
            for c in enode.children:
                epos=self.set_spans(c,fspane,epos)
                span=span_cover(span,c.span)
            enode.span=span
            return epos

    def find_frontier(self,enode,allow_epsilon_rhs=False,cspan=None):
        """set treenode.frontier_node iff (GHKM) span and cspan are nonoverlapping; cspan is union of spans of nodes that aren't
descendant or ancestor'.  cspan is a mutable array that efficiently tracks the current complement span by counting the number of
times each word is covered"""
        if cspan is None:
            cspan=[1]*self.nf
        spanr=range(enode.span[0],enode.span[1])
        if (spanr or allow_epsilon_rhs):
            fr=True
            for i in spanr:
                assert(cspan[i]>0)
                cspan[i]-=1
                if cspan[i]>0:
                    fr=False
            enode.frontier_node=fr
        else:
            enode.frontier_node=False
        for c in enode.children:
            for i in range(c.span[0],c.span[1]):
                cspan[i]+=1
        for c in enode.children:
            self.find_frontier(c,allow_epsilon_rhs,cspan)
        for c in enode.children:
            for i in range(c.span[0],c.span[1]):
                cspan[i]-=1
        for i in spanr:
            cspan[i]+=1

    def treenodes(self):
        return self.etree.preorder()

    def frontier(self):
        for c in self.treenodes():
            if c.frontier_node:
                yield c

    def ghkm(self,leaves_are_frontier=False,allow_epsilon_rhs=False):
        self.etree.span=(0,self.nf)
        self.find_frontier(self.etree,allow_epsilon_rhs)
        if not leaves_are_frontier:
            for c in self.etree.frontier():
                c.frontier_node=False
        for c in self.treenodes():
            c.allspan=c.span
            if not c.frontier_node:
                c.span=None

    @staticmethod
    def xrs_lhs_str(t,foreign,fbase,quote,xn=None):
        """return xrs rule lhs string, with the foreign words corresponding to the rhs span being replaced by the tuple (i,node)
where xi:node.label was a variable in the lhs string; only the first foreign word in the variable is replaced.  foreign initially is
foreign_whole_sentence[fbase:x], i.e. index 0 in foreign is at the first word in t.span.  xn is just an implementation detail (mutable cell); ignore it."""
        if xn is None: xn=[0]
        if t.is_terminal():
            return xrs_quote(t.label,quote)
        s=t.label+'('
        for c in t.children:
            if c.frontier_node:
                l=c.span[0]
                foreign[l-fbase]=(xn[0],c)
                s+=xrs_var_lhs(xn[0],c,quote)
                xn[0]+=1
            else:
                s+=Translation.xrs_lhs_str(c,foreign,fbase,quote,xn)
            s+=' '
        return s[:-1]+')'

    @staticmethod
    def xrsfrontier_generator(root):
        "generator: list of all frontier nodes below root with no other interposing frontier nodes"
        for c in root.children:
            if c.frontier_node:
                yield(c)
            else:
                for s in Translation.xrsfrontier(c):
                    yield s

    @staticmethod
    def xrsfrontier_rec(root,r):
        for c in root.children:
            if c.frontier_node:
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

    #TODO: bugfix with allow_epsilon_rhs
    @staticmethod
    def xrs_rhs_str(frhs,b,ge,quote=False):
        rhs=""
        gi=b
        while gi<ge:
            c=frhs[gi-b]
            if type(c) is tuple:
                rhs+=xrs_var(c[0])
                gi=c[1].span[1]
            else:
                rhs+=xrs_quote(c,quote)
                gi+=1
            rhs+=' '
        return rhs[:-1]

    def xrs_str(self,root,quote=False):
        assert(root.frontier_node)
        s=root.span
        b,ge=s
        frhs=self.f[b:ge]
        lhs=Translation.xrs_lhs_str(root,frhs,b,quote)
        rhs=Translation.xrs_rhs_str(frhs,b,ge,quote)
        return lhs+' -> '+rhs

    def all_rules(self,quote=False):
        "list of all minimal rules"
        return [self.xrs_str(c,quote) for c in self.frontier()]

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

class Training(object):
    def __init__(self,parsef,alignf,ff):
        self.parsef=parsef
        self.alignf=alignf
        self.ff=ff
    def __str__(self):
        return "[parallel training: e-parse=%s align=%s foreign=%s]"%(self.parsef,self.alignf,self.ff)
    def reader(self):
        for eline,aline,fline,lineno in itertools.izip(open(self.parsef),open(self.alignf),open(self.ff),itertools.count(0)):
                yield Translation.parse_sent(eline,aline,fline,lineno)

pod_types=[int,float,long,complex,str,unicode,bool]
def attr_pairlist(obj,names=None,types=pod_types):
    """return a list of tuples (a1,v1)... if names is a list ["a1",...], or all the attributes if names is None.  if types is not None, then filter the tuples to those whose value's type is in types'"""
    if not names:
        names=[a for a in dir(obj) if a[0:2] != '__']
    return [(k,getattr(obj,k)) for k in names if hasattr(obj,k) and (types is None or type(getattr(obj,k)) in types)]

def attr_str(obj,names=None,types=pod_types):
    "return string: a1=v1 a2=v2 for attr_pairlist"
    return ' '.join(["%s=%s"%p for p in attr_pairlist(obj,names,types)])

def main():
    opts,_=usage.parse_args()
    if opts.header:
        print "### gextract %s minimal %s"%(version,attr_str(opts,['terminals','quote','attr','derivation','inbase']))
        #"terminals=%s quote=%s attr=%s derivation=%s inbase=%s"%(opts.terminals,opts.quote,opts.attr,opts.derivation,opts.inbase)
    inbase=opts.inbase
    train=Training(inbase+".e-parse",inbase+".a",inbase+".f")
    for t in train.reader():
        t.ghkm(opts.terminals)
        print
        if opts.header:
            print "###",t
        for r,id in itertools.izip(t.all_rules(opts.quote),itertools.count(0)):
            print r+("### line=%d id=%d"%(t.lineno,id) if opts.attr else "")
        if (opts.derivation):
            print t.derivation_tree()

if __name__ == "__main__":
    errors=main()
    if errors: sys.exit(errors)
