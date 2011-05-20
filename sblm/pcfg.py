#!/usr/bin/env python2.6
"""
a simple PCFG. more complicated than you'd think.

papers: collins 97, bikel ('intricacies')
accurate unlexicalized parsing (klein+manning)

p(l->rhs|l) - but need smoothing

allow unseen terminals? yes (green rules)
unseen terminals and digit->@ trick both potentially give sum(prob)>1. but this is ok.

sri ngram style backoff - most specific prob, unsmoothed (pre-discounted)? context dependent backoff cost. (use indicator feature for bo cost, but not one for discounting unsmoothed). can xform linear weighted interpolation to sri style by precomputing the sums for all non-0count events.

witten-bell smoothing instead of indicators? witten-bell: c_ctx' = c_ctx/(c_ctx+k*u_ctx) where u_ctx is number of types that occur in ctx and c_ctx is the count of a particular type. k=5 in collins. actually he used c'=c(c+j+k*u), with (j,k)=(0,5) for everything, except for subcat=(5,0) and for p(w|')=(1,0).

indicator features for OOV terminals, (# of digits - no thanks).

allow unseen nonterminals? no (closed class). any new NT would be a bug.

hard rule: 2 types of nonterminals: preterminals and regular. preterminals generate exactly a single terminal; regular NTs never generate any terminals. so backoff from preterminal l vs nt l is different

backoff options:

usually generate lex|preterm, but if it's a new tag for the word, backoff to unigram p(lex).

order-n markov model for sequence l,r[1],r[2]...r[n],STOP. (simplest). always include l (optional: l-parent) in conditioning? always include r[1]?

or choose length n given l, then choose r[1..n] indepednently or order-n

remember to backoff to unigram p(nt|*) in worst case. count-based, not uniform (assumes training includes counts for all NTs).

and backoff to smoothed geometric dist. p(len|l), or else histogram p(len|*) w/ recourse to smoothed geom at last resort.

option: extend reach of production to height 2 when preterms are involved (i.e. treat tag/term as single node). still need same backoff (to height 1 etc)

"""

import re
import tree
from graehl import *
from dumpx import *
from ngram import *

headindex_re=re.compile(r'([^~]+)~(\d+)~(\d+)')
raduhead_base_skips=set(['.',"''",'``'])
raduhead_more_skips=set([':',','])
raduhead_skips=raduhead_base_skips.union(raduhead_more_skips)
raduhead_npb_skips=raduhead_base_skips
#,'NML','ADJP'
def raduhead(t,dbgmsg='',headword=True):
    m=headindex_re.match(t.label)
    t.label_orig=t.label
    if m is None:
        if headword and t.is_preterminal():
            t.headword=(t.label,t.children[0].label)
        return
    label,n,head=m.group(1,2,3)
    skips=(raduhead_npb_skips if label=='NPB' else raduhead_skips)
    #warn('raduhead','%s=%s,%s,%s'%(t.label,label,head,n))
    t.head=head=int(head)
    n=int(n)
    i=0
    #i==0 and c.label==':' or
    cn=[]
    for c in t.children:
        if c.label not in skips:
            cn.append(c)
        i+=1
    t.head_children=cn
    t.good_head=(n==len(cn) and head<=n and  head>0)
    if not t.good_head:
        warn('wrong head index for %s'%label,('%s!=%s %s => %s %s')%(n,len(cn),t.label,[c.label_orig for c in t.children],dbgmsg),max=None)
        if headword: t.headword=t.head_children[-1].headword
    elif headword:
        t.headword=t.head_children[head-1].headword
    t.label=label

def raduparse(tline,intern_labels=False,strip_head=True):
    t=radu2ptb(tline,strip_head=strip_head)
    t=tree.str_to_tree(t,intern_labels=intern_labels)
    if not strip_head:
        #warn('raduparse','%s=%s'%(tline,t.str(square=True)))
        for n in t.postorder():
            raduhead(n,dbgmsg=tline)
    return t

def is_bar(s):
    return len(s)>5 and s[0]=='@' and s[-4:]=='-BAR'

def strip_bar(s):
    if len(s)<5: return s
    if s[0]=='@' and s[-4:]=='-BAR': return s[1:-4]
    return s

def no_bar(s):
    return None if is_bar(s) else s

subcat_s=r'-\d+(?=$|-BAR)'
subcat_re=re.compile(subcat_s)
def is_subcat(s):
    return subcat_re.match(s)

def strip_subcat(s):
    return subcat_re.sub('',s)

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
        warn("PCFG backoff:",evs,max=10)
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

numre=re.compile(r"[0-9]")

def maybe_num2at(s,num2at=True):
    return numre.sub('@',s)  if num2at else s

# quote is quoted as """,  "" as """"". no other " are allowed. so just surround by " "
def pcfg_quote_terminal(s,num2at=True):
    return '"'+maybe_num2at(s,num2at)+'"'

def sbmt_lhs_label(t,num2at=True):
    return pcfg_quote_terminal(t.label,num2at) if t.is_terminal() else t.label_lrb()

#pcfg event is a nonempty list of [lhs]+[children]. this should be called on raw eng-parse, not sbmt rule lhs, which have already quoted leaves. we include terminal -> [] because we want unigram prob backoffs
def sbmt_lhs_pcfg_event(t,num2at=True):
    return [sbmt_lhs_label(c) for c in [t]+t.children]

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

def gen_pcfg_events_radu(t,terminals=False,terminals_unigram=False,digit2at=False):
#        dump(t)
    if t is None:
        return
#    dump(type(t),t)
    for n in t.preorder():
        ev=sbmt_lhs_pcfg_event(n,digit2at)
        use=False
#        term='terminal'
# return a tuple to distinguish terminals
        if n.is_terminal():
            if terminals_unigram: yield (ev,)
        elif n.is_preterminal():
            yield (ev,) #ret[0] = ev = (preterm,term)
        else:
            yield ev

class tag_word_unigram(object):
    "p(word|tag), with smoothing (by default OOV words are logp_unk=0, i.e. free)"
    def __init__(self,bo=0.1,digit2at=False,logp_unk=0.0):
        self.digit2at=digit2at
        self.tagword=IntDict() # IntDicts are reused as probs once normalized
        self.word=IntDict()
        self.bo=bo # witten bell constant
        self.bo_for_tag=dict()
        self.logp_unk=logp_unk # penalty added per unk word
        self.have_counts=True
        self.have_p=False
        self.have_logp=False
    def preterminal_vocab(self):
        "must have trained"
        return self.bo_for_tag.iterkeys()
    def terminal_vocab(self):
        return self.word.iterkeys()
    def ngram_lm(self):
        return build_2gram(self.tagword,self.word,self.bo_for_tag,digit2at=self.digit2at,logp_unk=self.logp_unk)
    def write_lm(self,file,sort=True):
        n=self.ngram_lm()
        n.write_lm(file=file,sort=sort)
    def oov(self,w):
        return not w in self.word
    def count_tw(self,tw):
        self.word[tw[1]]+=1
        self.tagword[tw]+=1
    def count(self,tag,word):
        self.word[word]+=1
        self.tagword[(tag,word)]+=1
    def logp(self,t,w):
        return self.logp_tw((t,w))
    def logp_tw(self,tw):
        return self.logp_tw_known(tw)[0]
    def logp_tw_known(self,tw):
        if tw in self.tagword:
            return (self.tagword[tw],1)
        else:
            t,w=tw
            if w in self.word:
                return (self.bo_for_tag[t]+self.word[w],1)
            return (self.logp_unk,0)
    def train(self,witten_bell=True,unkword='<unk>'):
        "unkword = None to not reserve prob mass for unk words"
        self.normalize(witten_bell,unkword)
        self.precompute_interp()
        self.to_logp()
    def normalize(self,witten_bell=True,unkword='<unk>'):
        tsums=FloatDict()
        ttypes=IntDict()
        wsum=0.0
        nw1=0.0
        for (t,w),c in self.tagword.iteritems():
            tsums[t]+=c
            ttypes[t]+=1 # for witten-bell
            wsum+=c
            if c==1:
                nw1+=1
        if unkword is not None:
            wsum+=nw1
        for w in self.word.iterkeys():
            c=self.word[w]
            self.word[w]=c/wsum
        for tw in self.tagword.iterkeys():
            t=tw[0]
            self.tagword[tw]=self.tagword[tw]/tsums[t]
        for t in ttypes.iterkeys():
            if witten_bell:
                sum=tsums[t]
                ntype=ttypes[t]
                self.bo_for_tag[t]=ntype/sum
            else:
                self.bo_for_tag[t]=self.bo
        if unkword is not None:
            punk=nw1/wsum
            self.word[unkword]=punk
            self.logp_unk=log_prob(punk)
        self.have_p=True
    def precompute_interp(self):
        "if you don't call this, then inconsistent model results"
        assert self.have_p
        for tw,p in self.tagword.iteritems():
            t,w=tw
            a=self.bo_for_tag[t]
            oldp=self.tagword[tw]
            self.tagword[tw]=(1-a)*oldp+a*self.word[w]
    def to_p(self):
        assert self.have_logp
        self.have_logp=False
        for k,p in self.word.iteritems():
            self.word[k]=math.exp(p)
        for k,p in self.tagword.iteritems():
            self.tagword[k]=math.exp(p)
        self.have_p=True
    def to_logp(self):
        assert self.have_p
        self.have_p=False
        for k,p in self.word.iteritems():
            if not p>0: warn("0 prob word: p(%s)=%s"%(k,p))
            self.word[k]=log10_prob(p)
        for k,p in self.tagword.iteritems():
            if not p>0: warn("0 prob tag/word: p(%s)=%s"%(k,p))
            self.tagword[k]=log10_prob(p)
        for k,p in self.bo_for_tag.iteritems():
            if not p>0: warn("0 prob tag backoff: p(%s)=%s"%(k,p))
            self.bo_for_tag[k]=log10_prob(p)
        self.have_logp=True
    def __str__(self,head=10):
        return ''.join(head_sorted_str(x.iteritems(),reverse=True,key=lambda x:x[1],head=head) for x in [self.tagword,self.word])

#from collections import defaultdict
import tempfile
class sblm_ngram(object):
    "require all tags in vocabulary. learn ngram model of sequence of children given parent (separate ngram model for each parent tag). interpolated with non-parent-specific ngram, which is not interpolated w/ 0-gram because every tag 1gram is observed somewhere in the whole training."
    spre='<s:'
    spost='>'
    start=intern('<s>')
    end=intern('</s>')
    unk='<unk>'
    def __init__(self,order=2,parent=False,digit2at=False,parent_alpha=0.99,cond_parent=False,parent_start=False,skip_bar=True,unsplit=True,logp_unk=0.0,witten_bo=0.1):
        """
        parent: use parent_alpha*p(children|parent)+(1-parent_alpha)*p(children)

        parent_start: make the <s> symbol <s:NP> - not needed if you use cond_parent or parent

        cond_parent: ngram c[0]...c[i-1] PARENT c[i] for each i you score. backs off to c[i]|PARENT and c[i]. overrides parent. (this is fine; they aim toward the same purpose)

        """
        self.parent_start=parent_start
        self.cond_parent=cond_parent
        self.parent=parent #distinct ngrams for each parent; backoff to indistinct
        self.digit2at=digit2at
        self.order=order
        self.ng=ngram(order,digit2at=False,logp_unk=logp_unk)
        self.png=dict()
        self.terminals=tag_word_unigram(bo=witten_bo,digit2at=digit2at,logp_unk=logp_unk) #simplify: use an ngram for terminals. tag_word_unigram is functionally equiv to bigram lm anyway
        self.unsplit=unsplit
        self.skip_bar=skip_bar
        self.unsplit_map=strip_subcat if unsplit else identity
        self.skip_map=no_bar if skip_bar else identity
        self.label_map=lambda l: self.skip_map(self.unsplit_map(l))
        self.set_parent_alpha(parent_alpha)
    def interp_no_parent(self,no_parent,with_parent):
        return math.log10(self.wt*10.**no_parent+self.pwt*10.**with_parent) #TODO: preinterpolate self.ng into self.png before taking logp and normalizing. or just preinterp for speed
    def set_parent_alpha(self,parent_alpha=0.99):
        self.pwt=parent_alpha
        if not self.parent: self.pwt=0.0
        self.wt=1.-self.pwt
    def preterminal_vocab(self):
        return self.terminals.preterminal_vocab()
    def nonterminal_vocab(self):
        return self.ng.vocab_ctx()
    def terminal_vocab(self):
        return self.terminals.terminal_vocab()
    def start_for_parent(self,p):
        return intern('<s:'+p+'>') if self.parent_start else sblm_ngram.start
    def sent_for_event(self,p,ch):
        # these come from already-interned tree labels
        sent=[self.start_for_parent(p)]+ch
        sent.append(sblm_ngram.end)
        return sent
    def score_children(self,p,ch):
        sent=self.sent_for_event(p,ch)
        bo=self.ng
        # warn("score_children",'%s => %s'%(p,sent),max=10)
        if self.cond_parent:
            s=0.
            ng=self.ng
            i=len(sent)
            sent.append(None)
            while i>1: # don't predict <s> | parent
                sent[i]=sent[i-1]
                sent[i-1]=p
                # warn("score_children cond_parent","%s"%(sent[:i+1]),max=10)
                lprob,bo=ng.score_word(sent,i)
                s+=(lprob+bo)
                i-=1
            return s
        elif self.parent and p in self.png:
            dist=self.png[p]
            s=0.
            pa=self.pwt
            for i in range(1,len(sent)):
                pp,pbo=dist.score_word(sent,i)
#                warn("score given parent",'%s=%s+%s=%s'%(sent[0:i+1],pp,pbo,pp+pbo),max=10)
                bp,bbo=bo.score_word(sent,i)
                                        #                warn("score forgetting parent",'%s=%s+%s=%s'%(sent[0:i+1],bp,bbo,bp+bbo),max=10)
                li=log10_interp(pp+pbo,bp+bbo,pa)
                if li==log10_0prob:
                    warn("score_children 0 interpolated prob","alpha=%s, p(%s|%s)=%s+%s p(|*)=%s+%s"%(pa,sent[:i+1],p,pp,pbo,bp,bbo))
                s+=li
#                s+=dist.score_word_interp(sent,i,bo,parent_alpha)
            return s
        else:
            return bo.score_text(sent,i=1)
    def eval_pcfg_event(self,e):
        "return (logp,n) where n is 1 if the pcfg rewrite was scored, 0 otherwise (because we skip unknown terminals, those get 0)"
        if type(e)==tuple:
            e=tuple(e[0])
            assert len(e)==2
#            warn("eval_pcfg terminal",e,max=10)
            return self.terminals.logp_tw_known(e)
        else:
            return (self.score_children(e[0],e[1:]),1) #len(e)-1
    def tree_from_line(self,line):
        return raduparse(line,intern_labels=True).map_skipping(self.label_map)
    def eval_radu(self,input):
        if type(input)==str: input=open(input)
        #FIXME: use gen_pcfg_events_radu
        logp=0.
        n=0
        nnode=0
        nunk=0
        nw=0
        ntrees=0
        unkwords=IntDict()
        for line in input:
#            n+=1 #TOP
            t=self.tree_from_line(line)
            if t is None:
                next
            if t.size()<=1: warn("eval_radu small tree",t,line,max=None)
            nnode+=t.size()
            nw+=len(t)
            ntrees+=1
            # warn("eval_radu",t,max=10)
            for e in gen_pcfg_events_radu(t,terminals=False,digit2at=self.digit2at):
                lp,nknown=self.eval_pcfg_event(e)
#                warn('pcfg_events_radu','p(%s)=%s%s'%(e,lp,'' if nknown else ' unk'),max=100)
                logp+=lp
                if nknown==0:
                    #warn("unk sblm_ngram eval terminal:",e,max=10)
                    unkwords[tuple(e[0])]+=1
                    nunk+=1
                else:
                    n+=nknown
        return dict(logprob=logp,nnode=nnode,nevents=n,ntrees=ntrees,nwords=nw,nunk=nunk,nunk_types=len(unkwords),top_unk=head_sorted_dict_val_str(unkwords,head=10,reverse=True))
    def read_radu(self,input):
        if type(input)==str: input=open(input)
        n=0
        for line in input:
            t=self.tree_from_line(line)
            if t is not None:
                n+=t.size()
            for e in gen_pcfg_events_radu(t,terminals=True,digit2at=self.digit2at):
                if type(e)==tuple:
                    e=tuple(e[0])
#                    warn("sblm_ngram train terminal",e,max=10)
                    self.terminals.count_tw(e)
                else:
                    if len(e)==0: continue
                    p=e[0]
                    sent=self.sent_for_event(p,e[1:])
                    if self.cond_parent:
                        i=len(sent)
                        sent.append(None)
                        while i>1:
                            sent[i]=sent[i-1]
                            sent[i-1]=p
                            # warn("read_radu cond_parent","%s => %s i=%s"%(p,sent[:i+1],i))
                            self.ng.count_word(sent,i)
                            i-=1
                    else:
                        self.ng.count_text(sent,i=1)
                        if self.parent:
                            pngs=self.png
                            if p not in pngs:
                                pn=ngram(self.order,digit2at=False)
                                pngs[p]=pn
                            else:
                                pn=pngs[p]
                                pn.count_text(sent,i=1)
        return n
    def check(self,epsilon=1e-5,uninterp=True):
        sum1=[]
        for p,n in chain(self.png.iteritems(),[('nts',self.ng),('preterm(word)',self.terminals.ngram_lm())]):
            n1,nlt1,gt1=n.check_sums(epsilon=epsilon,uninterp=uninterp)
            total=float(n1+nlt1+len(gt1))
            if gt1:
                warn("PCFG (%s): ngram sums for context >1:"%p)
                write_kv(gt1.iteritems(),out=sys.stderr)
            if total:
                if total==n1:
                    sum1.append(p)
                else:
                    log("PCFG (%s): sum=1:%d/(N=%d)=%s sum<1:%d/N=%s sum>1:%d/N=%s sum>1"%(p,n1,total,n1/total,nlt1,nlt1/total,len(gt1),len(gt1)/total))
        log("PCFG sum=1: "+' '.join(sum1))
    def train_lm(self,prefix=None,lmf=None,uni_witten_bell=True,uni_unkword=None,ngram_witten_bell=False,sri_ngram_count=False,check=True,write_lm=False,merge_terminals=True,sort=True):
        prefixterm=None
        if prefix is not None:
            prefixterm=prefix+'.terminals'
            prefix+='.pcfg'
        mkdir_parent(prefix)
        self.terminals.train(uni_witten_bell,uni_unkword)
        all=self.ng.train_lm(prefix=prefix,sort=sort,lmf=lmf,witten_bell=ngram_witten_bell,read_lm=True,sri_ngram_count=sri_ngram_count,write_lm=(write_lm and not merge_terminals))
        if merge_terminals:
            self.ng.disjoint_add_lm(self.terminals.ngram_lm(),conflict_take_bow_only=True) #NOTE: for preterms, we want the bow from the terminal model, and the backed off unigram prob from the PCFG rewrite model
            # without doing this we have from pcfg sblm (in merged result): -2.51869334212  VBP-0   -2.59911856506
            # but from terminal 2gram: -99     VBP-0   -2.73926926467
            # we want to use the -2.73 BO
            #anomolies: -0.000219339811938      <s> VBP-0 @VBP-0-BAR    -2.81056852922
            # should mean: VBP-0(@VBP-0-BAR ...)
            # -3.1224484856   @VBP-0-BAR VBP-0 DT-1   0
            # VBP-0(DT-1 @VBP-0-BAR)
            # -0.0103815795832        @VBP-0-BAR VBP-0 VBP-0  -3.11260500153
            # VBP-0(@VBP-0-BAR VBP-0)
            # what we could do if we wanted to make counts of as-NT vs as-PT compete: keep unigram backoff events for lex vs nonlex sep, but otherwise let bigram (given parent) compete? also need to choose-one-child <s> "lex" </s>?
            if write_lm:
                self.ng.write_lm(lmf,prefix=prefix,sort=sort)
        if write_lm and prefixterm is not None:
            self.terminals.write_lm(prefixterm)
        if self.parent:
            for nt,png in self.png.iteritems():
                #self.nonterminal_vocab()
                nt=intern(nt)
                pnt=filename_from_1to1('%s.%s'%(prefix,nt))
                png=self.png[nt]
                plmf=None if lmf is None else filename_from_1to1('%s.%s'%(lmf,nt))
                pntf=png.train_lm(prefix=pnt,sort=True,lmf=plmf,witten_bell=ngram_witten_bell,read_lm=True,sri_ngram_count=sri_ngram_count,write_lm=write_lm)
                #dump('lm for',nt,plmf)
    def __str__(self):
        return str(self.terminals)

dev='sample/dev.e-parse'
test='sample/test.e-parse'
train='sample/training.e-parse'
#train='training.e-parse'

def pcfg_ngram_main(n=5,
                    train=train
                    ,eval=False
                    ,dev=dev
                    ,test=test
                    ,parent=True
                    ,parent_alpha=0.999
                    ,cond_parent=True
                    ,witten_bell=True
#                    ,logfile="ppx.dev.txt"
                    ,eval_logfile="test.sri.txt"
                    ,sri_ngram_count=False
                    ,digit2at=True
                    ,write_lm=True
                    ,merge_terminals=True
                    ,skip_bar=True
                    ,unsplit=True
                    ,outpre=""
                    ,logp_unk=0.0
                    #,logp_glue=0.0 #what about GLUE0 etc?
                    ,bo_witten=0.1
                    ):
    log('pcfg_ngram')
    log(str(Locals()))
    sb=sblm_ngram(order=n,parent=parent,parent_alpha=parent_alpha,cond_parent=cond_parent,skip_bar=skip_bar,unsplit=unsplit,digit2at=digit2at,witten_bo=bo_witten,logp_unk=logp_unk)
    if len(outpre)==0: outpre=train
    #dumpx(str(sb.tree_from_line('(S-2 (@S-BAR (A "a"  ) (B-2 "b"  ) ) )')))
    ntrain=sb.read_radu(train)
    s=Stopwatch('Train')
    sb.train_lm(prefix=outpre+'.sblm/sblm',sri_ngram_count=sri_ngram_count,ngram_witten_bell=witten_bell,write_lm=write_lm,merge_terminals=merge_terminals)
    log('# training nodes: %s'%ntrain)
    log('lm file: %s'%sb.ng.lmfile())
    warn(s)
    #if write_lm and (not merge_terminals or True): sb.terminals.write_lm(outpre+'.terminals')
    #if not merge_terminals: sb.check(epsilon=1e-5)
    if False:
       write_list(sb.preterminal_vocab(),name='preterminals')
       write_list(sb.png.keys(),name='parents(NTs)')
       write_list(sb.nonterminal_vocab(),name='nonterminals')
       dump(sri.name)
       callv(['head',sri.name])
       print str(sb)
    if eval:
        for t in [dev,test]:
            s=Stopwatch('Score '+t)
            e=sb.eval_radu(t)
            warn(s)
            e['corpus']=t
            e['ngram-order']=n
            e['-logprob_2']=-log10_tobase(e['logprob'],2)
            e['-logprob_2/nnode']=e['-logprob_2']/e['nnode']
            del e['top_unk']
            write_dict(e)
            head=str(Locals())
            def outd(x):
                write_dict(e,out=x)
                x.write('\n')
            append_logfile(eval_logfile,outd,header=head)
    info_summary()

import optfunc
optfunc.main(pcfg_ngram_main)
#if __name__ == "__main__":
#    pcfg_ngram_main()

"""
TODO:

debug no-sri vs sri difference (done for now: close, but </s> gets diff unigram prob.)

load/save trained sblm and raw counts?

1-to-1 NT->filename mapping (for decoder feature)

decoder feature

check rules for @NP-BAR -> NP mapping

learn how to linear-interp (not loglinear) a bo-structure ngram. also loglinear. or just use srilm tool

heuristic+early scoring for parts of linear-interp models, where backoffs can be scored earlier? can do for loglinear-interp, maybe not linear-interp
"""
