import re
import tree
from graehl import *
from dumpx import *
def scan_sbmt_lhs(tokens,pos):
    "return (t,pos') with pos' one past end of recognized t"

rule_arrow_s='->'
#rule_arrow=re.compile(r"\s*\-\>\s*")
rule_bangs_s=r"###"
rhs_word_s=r'\S+'
rhs_word=re.compile(rhs_word_s)
#rule_bangs=re.compile(rule_bangs_s)
#featval=r"{{{.*?}}}|\S+"
#featname=r"[^ =]+"
#rule_feat=re.compile(r'\s*('+featname+')=('+featval+')')
rule_id_s=r"\bid=(-?\d+)\b"
rule_id=re.compile(rule_id_s)
find_id_whole_rule=re.compile(rule_bangs_s+".*"+rule_id_s)
lhs_token_s=r'\(|\)|[^\s"()]+|"""|""""|"[^"\s]*"' #DANGER: does this try/backtrack left->right? if so, ok. otherwise, problems?
lhs_tokens=re.compile(lhs_token_s)
space_re=re.compile(r'\s*')

def find_rule_id(s,iattr=0):
    m=rule_id.search(s,iattr)
    if not m:
        raise Exception("couldn't find id= in rule attributes %s"%s[iattr:])
    return m.group(1)

def tokenize(re,s,until_token=None,pos=0,allow_space=True):
    'return parallel lists [token],[(i,j)] so that s[i:j]=token. if allow_space is False, then the entire string must be consumed. otherwise spaces anywhere between tokens are ignored. until_token is a terminator (which is included in the return)'
    tokens=[]
    spans=[]
    while True:
        if allow_space:
            m=space_re.match(s,pos)
            if m: pos=m.end()
        if pos==len(s):
            break
        m=re.match(s,pos)
        if not m:
            raise Exception("tokenize: re %s didn't match string %s at position %s - remainder %s"%(re,s,pos,s[pos:]))
        pos2=m.end()
        t=s[pos:pos2]
        tokens.append(t)
        spans.append((pos,pos2))
        if t==until_token:
            break
        pos=pos2
    return (tokens,spans)

    #sep=False
    # for x in re.split(s):
    #     if sep: r.append(x)
    #     elif allow_space:
    #         if not space_re.match(x): return None
    #     else:
    #         if len(x): return None
    #     sep=not sep
    # return r

from dumpx import *

# return (tree,start-rhs-pos,end-tree-pos)
def parse_sbmt_lhs(s,require_arrow=True):
    (tokens,spans)=tokenize(lhs_tokens,s,rule_arrow_s)
    if not len(tokens):
        raise Exception("sbmt rule has no LHS tokens: %s"%s)
    (_,p2)=spans[-1]
    if tokens[-1] == rule_arrow_s:
        tokens.pop()
    elif require_arrow:
        raise Exception("sbmt rule LHS not terminated in %s: %s"%(rule_arrow_s,tokens))
    (t,endt)=tree.scan_tree(tokens,0,True)
    if t is None or endt != len(tokens):
        raise Exception("sbmt rule LHS tokens weren't parsed into a tree: %s TREE_ENDS_HERE unparsed = %s"%(tokens[0:endt],tokens[endt:]))
    (_,p1)=spans[endt]
    return (t,p2,p1)

# return (tree,[rhs-tokens],start-attr-pos,end-rhs-pos,start-rhs-pos,end-tree-pos). s[start-attr-pos:] are the unparsed attributes (if any) for the rule
def parse_sbmt_rule(s,require_arrow=True,require_bangs=True): #bangs = the ### sep. maybe you want to parse rule w/o any features.
    (t,p2,p1)=parse_sbmt_lhs(s,require_arrow)
    (rhs,rspans)=tokenize(rhs_word,s,rule_bangs_s,p2)
    if len(rhs):
        (_,p4)=rspans[-1]
        if rhs[-1]==rule_bangs_s:
            rhs.pop()
            (_,p3)=rspans[-2]
        elif require_bangs:
            raise Exception("sbmt rule RHS wasn't terminated in %s: %s"%(rule_bangs_s,s[pos:]))
        else:
            p3=p4
    else:
        p3=p2
        p4=p2
#    dumpx(str(t),rhs,p1,p2,p3,s[p3:])
    return (t,rhs,p4,p3,p2,p1)


#parse output from cat-pcfg-divide: e.g. @ADJP-0-BAR @ADJP-0-BAR ADJP-0	6	377 (event TAB count TAB norm-lhs-sum)
#return (eventstring,count,norm,cost=-log10(
import math

def prob2cost(p):
    return -math.log10(p)

def counts2cost(c,N):
    return prob2cost(float(c)/float(N))

def parse_pcfg_line(line):
    #return (ev,c,N)
    (ev,c,N)=line.split("\t")
    return (ev,float(c),float(N))
#    return (ev,c,N,counts2cost(c,N))

# def parse_pcfg_total(line,name):
#     dump(line,name)
#     (n,t)=line.split("\t")
#     if n!=name:
#         raise Exception("Expected line with %s N, but got instead:\t%s",(name,line))
#     return float(t)

def cost2str(val,asprob=False):
    if asprob:
        return '10^%s' % -val
    else:
        return str(val)

#return [(name,val)] or [] if sparse
def feats_pairlist(name,val,opts):
    if name and (not opts.sparse or val):
        return [(name,cost2str(val,opts.asprob))]
    else:
        return []


def event2str(l):
    return ' '.join(l)

featspecial=re.compile(r'[=\s]')
def escape_featurename(s):
    return featspecial.sub('_',s)

#no smoothing yet; expect every event in rules to also occur in training data (EXCEPT GREEN RULES etc). we will return count of not-found events when we score a rule (sep. feature)
# also note: assumes lex items can only occur under preterms, which can only have a single lex child. otherwise overstates unigram bo prob.
total_nt="(TOTAL_NT)"
total_lex="(TOTAL_LEX)"
class PCFG(object):
    def read(self,file,pcfg_featname_pre='pcfg'):
        "set probabilities according to file lines: @ADJP-0-BAR @ADJP-0-BAR ADJP-0	6	377 (event TAB count TAB norm-lhs-sum)"
        self.featname=pcfg_featname_pre
        self.file=file
        self.prob={}
        self.bonames={}
        f=open(file)
        n=0
        self.nnt=None
        self.nlex=None
#        lexprob={} #map unigram lex item -> count. TODO: modify cat-pcfg-for-divide keys for total_nt, total_lex so they come first, or put in sep. file, so we don't have to save all the lex items.
        for line in f:
            n+=1
            try:
                (evs,c,lhs_sum)=parse_pcfg_line(line)
            except:
                raise Exception("PCFG line %s. wrong number of fields: %s"%(n,line))
            if evs==total_nt:
                self.nnt=lhs_sum
                continue
            if evs==total_lex:
                self.nlex=lhs_sum
                continue
            ev=evs.split(' ')
            lhs=ev[0]
            islex=evs[0:1]=='"'
            p=counts2cost(c,lhs_sum)
            if islex:
                if len(ev)!=1: # or lhs in lexprob
                    raise Exception("PCFG line %s: expected to see quoted lexical items as lhs in leaf (no rhs) PCFG events, once only. saw bad event %s on line:\n%s"%(n,evs,line))
#                lexprob[lhs]=p
            self.prob[evs]=p
            self.bonames[lhs]=1
            self.prob[lhs]=lhs_sum # will normalize below
        for k in self.bonames.iterkeys():
            self.bonames[k]=pcfg_featname_pre+'-bo-'+escape_featurename(k)
            self.prob[k]=counts2cost(self.prob[k],self.nnt)
#        for w,n in lexprob.iteritems():
#            assert(w not in self.prob)
#            self.prob[w]=counts2cost(lexprob[w],self.nlex)
        self.oovfeat=pcfg_featname_pre+'-oov'
    def __init__(self,file,pcfg_featname_pre='pcfg'):
        self.read(file,pcfg_featname_pre)
    def cost(self,ev):
        "return (cost(sum cost(child) or cost(event if found)),oov(= # of unseen children, implies unigram bo),bo(=None if no backoff, else label of lhs NT) - bo means oov=0, and conversely, i.e. there's no markovization BO (yet)"
        if ev is None:
            return (0,0,None)
        evs=event2str(ev)
        if evs in self.prob:
            return (self.prob[evs],0,None)
        bo=0
        oov=0
        warn("PCFG backoff: "+evs)
        for r in ev[1:]:
            if r in self.prob:
                bo+=self.prob[r]
            else:
                warn("PCFG OOV: "+r)
                oov+=1
        return (bo,oov,ev[0])
    def accum_costs(self,costs):
        nbolhs=IntDict()
        f=self.featname
        sumc=0.0
        sumoov=0
        for c,oov,bolhs in costs:
            sumoov+=oov
            sumc+=c
            if bolhs is not None:
                nbolhs[bolhs]+=1
        r=[(f,sumc)]
        if sumoov:
            r.append((self.oovfeat,sumoov))
        for l,n in nbolhs.items():
            r.append((self.bonames[l],n))
        return r
    # sparse pairlist [(fid,fval)]
    def cost_feats(self,ev):
        (c,boc,oov,bolhs)=self.cost(ev)
    def boname(root):
        if root is None:
            return 'pcfg_bo_NONE'
        return 'pcfg_bo_'+root
def parse_sbmt_label(t):
    if t.is_terminal(): return '"%s"'%t.label
    return t.label_lrb()

#pcfg event is a nonempty list of [lhs]+[children]. this should be called on raw eng-parse, not sbmt rule lhs, which have already quoted leaves. we include terminal -> [] because we want unigram prob backoffs
def parse_pcfg_event(t):
    if t.is_terminal(): return ['"%s"'%t.label]
    return [t.label_lrb()]+[parse_sbmt_label(c) for c in t.children]

varre=re.compile(r"^x\d+:")
def strip_var(l):
    return varre.sub('',l)

def lhs_label(t):
    l=t.label_lrb()
    if t.is_terminal():
        sl=strip_var(l)
        if l!=sl: #variable
            return sl
        return t.label #parens are left alone in "leaf()((words"
    else:
        return l

def lhs_pcfg_event(t):
    assert(not t.is_terminal())
    return [t.label_lrb()]+[lhs_label(c) for c in t.children]
