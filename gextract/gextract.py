#!/usr/bin/env python2.6
"""
minimal ghkm rule extraction w/ ambiguous attachment (unaligned f words) -> highest possible node
"""

import os,sys,itertools,re
sys.path.append(os.path.dirname(sys.argv[0]))

import tree

import optparse
usage=optparse.OptionParser()
usage.add_option("-r","--inbase",dest="inbase",default="astronauts",help="input .e-parse .a .f")

def intpair(stringpair):
    return (int(stringpair[0]),int(stringpair[1]))

def xrs_quote(s):
    return '"'+s+'"'

def xrs_var(i):
    return 'x'+str(i)

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

def span_cover(sa,sb):
    "return smallest span covering both sa and sb; if either is 0-length then the other is returned (i.e. 0-length spans are empty sets, not singleton point sets)"
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

class Psent(object):
    def __init__(self,etree,estring,a,f):
        self.etree=etree
        self.estring=estring
        self.a=a
        self.f=f
        self.nf=len(f)
        self.ne=len(estring)
        assert(self.nf==a.nf)
        assert(self.ne==a.ne)
        self.fspane=a.spanadje()
        self.ghkm()

    def set_spans(self,enode,epos=0):
        "epos is enode's yield's starting position in english yield; returns ending position.  sets treenode.span foreign (a,b)"
        if enode.is_terminal():
            enode.span=self.fspane[epos]
            return epos+1
        else:
            span=None
            for c in enode.children:
                epos=self.set_spans(c,epos)
                span=span_cover(span,c.span)
            enode.span=span
            return epos

    def find_frontier(self,enode,cspan=None):
        """set treenode.frontier_node iff (GHKM) span and cspan are nonoverlapping; cspan is union of spans of nodes that aren't
descendant or ancestor'.  cspan is a mutable array that efficiently tracks the current complement span by counting the number of
times each word is covered"""
        if cspan is None:
            cspan=[1]*self.nf
        spanr=range(enode.span[0],enode.span[1])
        fr=True
        for i in spanr:
            cspan[i]-=1
            if cspan[i]:
                fr=False
        enode.frontier_node=fr
        for c in enode.children:
            for i in range(c.span[0],c.span[1]):
                cspan[i]+=1
        for c in enode.children:
            self.find_frontier(c,cspan)
        for c in enode.children:
            for i in range(c.span[0],c.span[1]):
                cspan[i]-=1
        for i in spanr:
            cspan[i]+=1

    def treenodes(self):
        return self.etree.postorder()

    def frontier(self):
        return [c for c in self.treenodes() if c.frontier_node]

    def ghkm(self,leaves_are_frontier=False):
        self.set_spans(self.etree)
        self.etree.span=(0,self.nf)
        self.find_frontier(self.etree)
        if not leaves_are_frontier:
            for c in self.etree.frontier():
                c.frontier_node=False
        for c in self.treenodes():
            if not c.frontier_node:
                c.span=None

    @staticmethod
    def xrsfrontier(root):
        for c in root.children:
            if c.frontier_node:
                yield(c)
            else:
                xrsfrontier(c)

    @staticmethod
    def xrs_lhs_str(t,foreign,fbase,xn=None):
        """return xrs rule lhs string, with the foreign words corresponding to the rhs span being replaced by the tuple (i,node)
where xi:node.label was a variable in the lhs string; only the first foreign word in the variable is replaced.  foreign initially is
foreign_whole_sentence[fbase:x], i.e. index 0 in foreign is at the first word in t.span.  xn is just an implementation detail (mutable cell); ignore it."""
        if xn is None: xn=[0]
        if t.is_terminal():
            return xrs_quote(t.label)
        s=t.label+'('
        for c in t.children:
            if c.frontier_node:
                l=c.span[0]
                foreign[l-fbase]=(xn[0],c)
                s+=xrs_var(xn[0])+':'+c.label
                xn[0]+=1
            else:
                s+=Psent.xrs_lhs_str(c,foreign,fbase,xn)
            s+=' '
        return s[:-1]+')'

    @staticmethod
    def xrs_rhs_str(frhs,b,ge):
        rhs=""
        gi=b
        while gi<ge:
            c=frhs[gi-b]
            if type(c) is tuple:
                rhs+=xrs_var(c[0])
                gi=c[1].span[1]
            else:
                rhs+=xrs_quote(c)
                gi+=1
            rhs+=' '
        return rhs[:-1]

    def xrs_str(self,root):
        assert(root.frontier_node)
        s=root.span
        b,ge=s
        frhs=self.f[b:ge]
        lhs=Psent.xrs_lhs_str(root,frhs,b)
        rhs=Psent.xrs_rhs_str(frhs,b,ge)
        return lhs+' -> '+rhs

    def all_rules(self,include_terminals=False):
        return [self.xrs_str(c) for c in self.frontier() if include_terminals or not c.is_terminal()]

    @staticmethod
    def fetree(etree):
        return etree.relabel(lambda t:t.label+span_str(t.span))

    def __str__(self):
        return "e={%s} #e=%d #f=%d a={%s} f={%s}"%(Psent.fetree(self.etree),self.ne,self.nf,self.a,self.f)

    def estring():
        return
    @staticmethod
    def parse_sent(eline,aline,fline):
        etree=raduparse(eline)
        e=etree.yieldlist()
        f=fline.strip().split()
        a=Alignment(aline,len(e),len(f))
        return Psent(etree,e,a,f)

class Training(object):
    def __init__(self,parsef,alignf,ff):
        self.parsef=parsef
        self.alignf=alignf
        self.ff=ff
    def __str__(self):
        return "[parallel training: e-parse=%s align=%s foreign=%s]"%(self.parsef,self.alignf,self.ff)
    def reader(self):
        for eline,aline,fline,lineno in itertools.izip(open(self.parsef),open(self.alignf),open(self.ff),itertools.count(0)):
                yield Psent.parse_sent(eline,aline,fline)

def main():
    opts,_=usage.parse_args()
    import dumpx
    inbase=opts.inbase
    train=Training(inbase+".e-parse",inbase+".a",inbase+".f")
    print "### gextract minimal inbase="+opts.inbase,train
    for t in train.reader():
        print
        print "### ",t
        for r in t.all_rules():
            print r

if __name__ == "__main__":
    errors=main()
    if errors:
        sys.exit(errors)
