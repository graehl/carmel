#!/usr/bin/env python
from graehl import *
from dumpx import *
import re
import subprocess
import os
import tempfile

default_ngram_count='ngram-count'

#intention: just those of a given order
class ngram_counts(object):
    def __init__(self,order=1):
        self.counts=IntDict()
        self.order=order
        self.om1=order-1
        self.ncountn=None
    def add_zero(self,k):
        if not k in self.counts:
            self.counts[k]=0
    def disjoint_add(self,ng,ignore_conflict=True,warn_conflict=True):
        "ng has ngrams that are disjoint from ours. add them. ignore = keep our values"
        disjoint_add_dict(self.counts,ng.counts,ignore_conflict=ignore_conflict,warn_conflict=warn_conflict,desc='ngram_counts disjoint_add')
    def add_i(self,vec,i):
#        dump('add_i',i,vec[i],self.om1)
        if i>=self.om1:
            self.add(vec[i-self.om1:i+1])
    def add(self,seq):
        key=intern_tuple(seq)
        self.counts[key]+=1
#        dump('add',seq,self.counts[key])
    def __repr__(self):
        return obj2str(self,['order','count'],None)
    def counts_of_counts(self,max=4):
        ncountn=[0]*(max+1)
        for c in self.counts.itervalues():
            if c<=max:
                ncountn[c]+=1
        return ncountn
    def compute_ncountn(self,max=4):
        if self.ncountn is None or len(self.ncountn)<=max:
            self.ncountn=self.counts_of_counts(max)
        return self.ncountn
    def write_kn_discounts(self,out,mincount=1):
        if isinstance(out, str): out=open(out,'w')
        def wf(name,val):
            out.write(('%s %s\n'%(name,val)))
        wf('mincount',mincount)
        nc=self.compute_ncountn()
        (c1,c2,c3,c4)=[max(float(x),0.5) for x in nc[1:5]] # max prevents div by 0 for small corpora
        y=c1/(c1+2*c2)
        wf('discount1',1-2*y*c2/c1)
        wf('discount2',2-3*y*c3/c2)
        wf('discount3+',3-4*y*c4/c3)
        out.flush()
    def write_counts(self,out,sort=True):
        if isinstance(out, str): out=open(out,'w')
        def wf(ngram,val):
            out.write(' '.join(ngram)+' '+str(val)+'\n')
        if sort:
            keys=sorted(self.counts.keys())
            for k in keys:
                wf(k,self.counts[k])
        else:
            for (ng,c) in self.counts.iteritems():
                wf(ng,c)
        out.flush()
    def write_counts_kndisc(self,prefix,sort=True):
        cf=prefix+'.counts'
        kf=prefix+'.kndisc'
        self.write_counts(cf,sort)
        self.write_kn_discounts(kf)
        return (cf,kf)
    def witten_bell_bos(self,log10=True):
        "return dict[context]=bo_prob"
        types=IntDict()
        sums=FloatDict()
        for k,count in self.counts.iteritems():
            c=k[:-1]
#            warn("wb bo count",'k=%s,count=%s,c=%s'%(k,count,c),max=10)
            types[c]+=1
            sums[c]+=count
        for k,t in types.iteritems():
            p=t/(t+sums[k])
            types[k]=math.log10(p) if log10 else p
#            warn("wb bo",'k=%s,t=%s,bo[k]=%s'%(k,t,types[k]),max=10)
        return types
    def sum_counts(self):
        return sum(float(x) for x in self.counts.itervalues())
    def sum_counts_ctx(self):
        sums=FloatDict()
        for k,count in self.counts.iteritems():
            sums[k[:-1]]+=count
        return sums
    def probs(self,preserve_counts=True,log10=True):
        if preserve_counts: c=self.counts.copy()
        self.to_probs(log10=log10)
        r=self.counts
        if preserve_counts: self.counts=c
        return r
    def to_probs(self,log10=True):
        sumc=self.sum_counts_ctx()
        for k,count in self.counts.iteritems():
            s=sumc[k[:-1]]
            prob=count/s if s else 0
#            if k[-1]=='</s>': warn('p[%s]=%s/%s=%s'%(k,count,s,count/s))
            self.counts[k]=log10_prob(prob) if log10 else prob

def shorten_context(t):
    return t[1:]

def copy_map_key(dic,mapf=identity):
    r=dict()
    for k,v in dic.iteritems():
        r[mapf(k)]=v
    return r

def untuple_safe(s):
    return s[0] if isinstance(s, tuple) else s

def entuple_safe(x):
    return x if isinstance(x, tuple) else (x,)

def build_2gram(logp2,logp1,bow1,digit2at=False,unkword=None,logp_unk=0.0):
    n=ngram(2,digit2at=digit2at,unkword=unkword,logp_unk=logp_unk)
    n.logp[1]=logp2
    n.logp[0]=copy_map_key(logp1,entuple_safe)
    n.bow[0]=copy_map_key(bow1,entuple_safe)
    return n

def build_2gram_counts(c2,c1=None,include_c2_ctx_in_c1=True,digit2at=False,unkword=None,logp_unk=0.0):
    n=ngram(2,digit2at=digit2at,unkword=unkword,logp_unk=logp_unk)
    if c1==None:
        c1=IntDict()
    if include_c2_ctx_in_c1:
        for k,count in c2.iteritems():
            ctx=k[0]
            c1[ctx]+=count
    n.ngrams[1]=c2
    n.ngrams[0]=copy_map_key(c1,entuple_safe)

class ngram(object):
    #log10(prob) and log10(bow)
    sos=intern('<s>')
    eos=intern('</s>')
    unk=intern('<unk>')
    specials=set((sos,eos,unk))
    def disjoint_add_lm(self,ng,ignore_conflict=True,warn_conflict=True,conflict_take_bow_only=False,merge_logp=take_first,merge_bow=take_first):
        if conflict_take_bow_only:
            merge_bow=merge_logp=take_first
        "ng has ngrams that are disjoint from ours. add them. ignore = keep our values"
        for o in range(0,min(self.order,ng.order)):
            self.ngrams[o].disjoint_add(ng.ngrams[o],ignore_conflict=ignore_conflict,warn_conflict=warn_conflict)
            disjoint_add_dict(self.logp[o],ng.logp[o],merge_fn=merge_logp,ignore_conflict=ignore_conflict,warn_conflict=warn_conflict,desc='ngram logp order=%s'%(o+1))
            disjoint_add_dict(self.bow[o],ng.bow[o],merge_fn=merge_bow,ignore_conflict=ignore_conflict,warn_conflict=warn_conflict,desc='ngram bow order=%s'%(o+1))
    @staticmethod
    def is_special(w):
        return w in ngram.specials
    #w==ngram.sos || w==ngram.eos
    def interp(self):
        "store as p(w|ctx): bo(ctx)*p(w|ctx[0:-1])+(1-bo(ctx)*p(w|ctx)"
        for o in range(0,self.om1):
            h=o+1
            lp=self.logp[h]
            bo=self.bow[o]
            lpbo=self.logp[o]
            for k,l in lp.iteritems():
                lp[k]=log10_interp(lpbo[k[1:]],l,10.**bo[k[:-1]])
    def uninterp(self):
        "undo the effect of self.interp()"
        for o in reversed(range(0,self.om1)):
            h=o+1
            lp=self.logp[h]
            bow=self.bow[o]
            lpbo=self.logp[o]
            for k,l in lp.iteritems():
                pcomb=10.**l
                b=10.**lpbo[k[1:]]
                bo=10.**bow[k[:-1]]
                p=(pcomb-bo*b)/(1.-bo)
#                warn("uninterp",'p_uninterp(%s)=[%s-%s*%s]/(1.-%s)=%s'%(k,pcomb,bo,b,bo,p))
                lp[k]=math.log10(p)
    def compute_uniform(self):
        pass
    #     self.uniform_p=1./len(self.logp[0])
    #     self.uniform_log10p=math.log10(self.uniform_p)
    # def uniform_p(self,_):
    #     return self.uniform_p
    # def uniform_log10p(self,_):
    #     return self.uniform_log10p
    def clear_counts(self):
        self.ngrams=[ngram_counts(o+1) for o in range(0,self.order)] #note: 0-indexed i.e. order of ngrams[0]==1
    def set_order(self,o):
#        dump("set_order",o)
        self.order=o
        self.om1=o-1
        self.logp=[dict() for _ in range(0,o)]
        self.bow=[dict() for _ in range(0,o)] # last bow is empty dict
        self.clear_counts()
        asserteq(len(self.logp),self.order)
        asserteq(len(self.bow),self.order)
    def __init__(self,order=2,digit2at=False,unkword=None,logp_unk=log10_0prob,lm=None,closed=False):
        self.closed=closed
        self.unkword=ngram.unk if unkword is None else unkword
        self.logp_unk=logp_unk
        self.digit2at=digit2at
        self.order=order
        if lm is not None:
            self.read_lm(lm,clear_counts=True)
        else:
            self.set_order(order)
    def count_text(self,text,i=0,pre=None,post=None):
        if isinstance(text, str): text=text.split()
        if self.digit2at: text=map(digit2at,text)
        if pre is not None:
            text=[pre]+text
            i+=1
        if post is not None: text.append(post)
        l=len(text)
        for w in range(i,l):
            self.count_word(text,w)
        # for o in range(0,self.order):
        #     cn=self.ngrams[o]
        #     start=o if pre is None else o+1
        #     for w in range(start,l):
        #         cn.add_i(text,w)
    def count_word(self,text,i):
        # warn('count_word',text[:i+1])
        for o in range(0,min(self.order,i+1)):
            self.ngrams[o].add_i(text,i)
    def set_closed(self,closed=True):
        self.closed=closed
    def score_word(self,text,i):
        "private (perform digit2at yourself) returns (logp,bo) for most specific ngram, where bo is the total of contexts' backoffs. text is a tuple"
        w=text[i]
        if self.closed:
            #warn('closed:unk?','%s = %s'%(w,self.is_unk(w)),max=10)
            if self.is_unk(w): return (self.logp_unk,0)
        e=i+1
        bo=0.
        maxom1=min(self.om1,i)
        for o in range(maxom1,-1,-1):
            l=i-o
            phrase=tuple(text[l:e])
            lp=self.logp[o]
            # warn('score_word','(text=%s i=%s o=%s phrase=%s %s'%(text,i,o,phrase,phrase in lp),max=10)
            if phrase in lp:
                return (lp[phrase],bo)
            elif o>0:
                bos=self.bow[o-1]
                bo+=bos.get(tuple(text[l:i]),0.0)
            else:
                return (self.logp_unk,bo)
    def score_word_combined(self,text,i):
        p,bo=self.score_word(text,i)
        return p+bo
    def score_word_interp(self,text,i,other_lm,self_wt):
        return log10_interp(self.score_word_combined(text,i),other_lm.score_word_combined(text,i),self_wt)
    def score_text_detail(self,text,i=0):
        "returns list of score,bo,score,bo (2x length of text[i:-1]). applies digit2at"
        if self.digit2at: text=map(digit2at,text)
        text=intern_tuple(text)
        return flatten(self.score_word(text,j) for j in range(i,len(text)))
    def score_text(self,text,i=0):
        return sum(self.score_text_detail(text,i=i))
    def count_file(self,inf,pre='<s>',post='</s>'):
        if isinstance(inf, str): inf=open(inf)
        for l in inf:
            self.count_text(l,0,pre,post)
    def __str__(self):
        return self.str(False)
#        return obj2str(self,['order'])
    def str(self,show_counts=True):
        attr=['order','logp_unk']
        if show_counts:
            attr.append('ngrams')
        return obj2str(self,attr,None)
    def write_counts_kndisc(self,prefix,sort=True):
        r=[]
        for o in range(0,self.order):
            r.append(self.ngrams[o].write_counts_kndisc('%s.%sgram'%(prefix,o+1),sort))
        return r
    def lmfile(self,prefix=None):
        if self.lmf is None:
            if prefix is None:
                tf=tempfile.NamedTemporaryFile()
                self.lmf=tf.name
                tf.close()
            else:
                self.lmf='%s.%sgram.srilm'%(prefix,self.order)
        return self.lmf
    def train_lm(self,prefix=None,sri_ngram_count=False,sort=True,lmf=None,witten_bell=False,read_lm=True, clear_counts=True,write_lm=False,interpolate=True):
        "mod K-N unless witten_bell. lmf is written if lm!=None or if sri_ngram_count=True or write_lm=True"
        self.lmf=lmf
        if lmf is None and (sri_ngram_count or write_lm):
            lmf=self.lmfile(prefix)
        if sri_ngram_count:
            sri_ngram_count=default_ngram_count
            cks=self.write_counts_kndisc(prefix,sort)
            cfs=[c for (c,k) in cks]
            #scfs=map(shellquote,cfs)
            kfs=[k for (c,k) in cks]
            knns=['-kn%s %s'%(i+1,shellquote(kfs[i])) for i in range(0,self.order)]
            knargs=' '.join(knns)
            #            knargs='-kndiscount'
            #-cdiscount 0 -addsmooth 0
            nosmoothargs='-prune 0 -gtmin 0 -gtmax 0'
            #'-no-sos -no-eos'
            smoothargs='-wbdiscount' if witten_bell else knargs
            smoothargs=nosmoothargs+' '+smoothargs
            interpargs='-interpolate' if interpolate else ''
            cmd="cat %s | %s -order %s %s -read - %s -lm %s"%(' '.join(cfs),sri_ngram_count,self.order,interpargs,smoothargs,shellquote(lmf))
            log(cmd)
            system_force(cmd)
            if read_lm:
                self.read_lm(lmf,clear_counts)
        else:
            for o in range(self.order-2,-1,-1):
                if witten_bell:
                    bos=self.ngrams[o+1].witten_bell_bos(log10=True)
                    self.bow[o]=bos
                    ps=self.ngrams[o].counts
                    for k in bos.iterkeys():
                        if not k in ps:
                            ps[k]=0 # feed this forward without setting a bogus prob entry?
                else:
                    assert False
            for o in range(0,self.order):
                self.logp[o]=self.ngrams[o].probs(preserve_counts=not clear_counts,log10=True)
            if interpolate:
                self.interp()
            if lmf:
                self.write_lm(lmf)
        if clear_counts:
            self.clear_counts()
        self.prepare()
        return lmf
    def vocab(self,exclude_specials=True):
        "pre: trained"
        s=set(x[0] for x in self.logp[0].iterkeys())
        if exclude_specials: s-=ngram.specials
        return s
    def vocab_ctx(self,exclude_specials=True):
        "pre: trained"
        s=set(x[0] for x in self.bow[0].iterkeys())
        if exclude_specials: s-=ngram.specials
        return s
#        for w in self.counts[0].iterkeys():
#            if not ngram.is_special(w):
#                yield w
    def write_vocab(self,outf):
        "pre: counts not cleared"
        if isinstance(outf, str): outf=open(outf,'w')
        counts=self.ngrams[0].counts
        if not len(counts):
            counts=self.bow[0]
        for k in counts.iterkeys():
            outf.write(k+'\n')
    def is_unk(self,w):
        """only call on trained LM"""
        return w==self.unkword or (w,) not in self.logp[0]
        # counts=self.ngrams[0].counts
        # if not len(counts):
        #     counts=self.logp[0]
        # return w==self.unkword or (w,) not in counts
    def read_lm(self,infile,clear_counts=True,order=None,read_unkp=True):
        if order is not None and order!=self.order:
            self.set_order(order)
        elif clear_counts:
            self.clear_counts()
        if isinstance(infile, str): infile=open(infile)
        grams=re.compile(r'\\(\d+)-grams:\s*')
        headgrams=re.compile(r'^ngram (\d+)=(\d+)')
        n=0
        logp=None
        bow=None
        maxo=1
        for line in infile:
            line=line.rstrip()
            if logp is None:
                h=headgrams.match(line)
                if h:
                    maxo=int(h.group(1))
                    #dump(h,maxo)
                    continue
            m=grams.match(line)
            if m:
                if logp is None and order is None:
                    self.set_order(maxo)
                n=int(m.group(1))
                log("reading %d-grams ..."%n)
                logp=self.logp[n-1]
                if n==self.order:
                    bow=None
                else:
                    bow=self.bow[n-1]
            elif logp is not None:
                fields=line.split('\t')
                if len(fields)>1:
                    phrase=intern_tuple(fields[1].split(' '))
#                    dump(phrase,fields,n,self.order)
                    logp[phrase]=float(fields[0])
                    if bow is not None:
                        b=0. #because </s> has no bow
                        if len(fields)>2:
                            b=fields[2]
                        elif phrase!=(ngram.eos,) and phrase!=(ngram.sos,):
                                warn("missing bow for (non-<s>-or-</s>) phrase "+fields[1])
                        bow[phrase]=float(b)
            else:
                if logp is not None and len(line): warn("skipped nonempty line "+line)
        if self.logp is None:
            self.set_order(max(maxo,1 if order is None else order))
            raise "Couldn't read lm %s"%infile
        #self.logp=logp
        self.prepare()
        if read_unkp:
            self.get_logp_unk_from_ngram(self.logp_unk)
        else:
            self.set_logp_unk()
        return infile
    def prepare(self):
        self.compute_uniform()
    def ngramkeys(self,order):
        order-=1
        assert(order>=0 and order<=self.om1)
        return self.logp[order].iterkeys() if order==self.om1 else iterkeys2(self.logp[order],self.bow[order])
    def check_sums(self,epsilon=1e-5,uninterp=False):
        """less than 1 sum given context is ok in a pre-interpolated LM or a BO LM; more than 1 is never ok.
        return (# =~ 1, # < 1, [dict[ctx]=sum>1])
        """
        if uninterp: self.uninterp()
        S=FloatDict()
        for o in range(0,self.order):
            lp=self.logp[o]
            for k,lp in lp.iteritems():
                if lp!=log10_0prob:
                    S[k[:-1]]+=10.**lp
        n1=0
        nlt1=0
        gt1=dict()
        for k,z in S.iteritems():
            if approx_equal(z,1,epsilon):
                n1+=1
            elif definitely_gt(z,1,epsilon):
                gt1[k]=z
            else:
                nlt1+=1
        if uninterp: self.interp()
        return (n1,nlt1,gt1)
    def set_logp_unk(self,logp_unk=None):
        if logp_unk is None:
            logp_unk=self.logp_unk
        else:
            self.logp_unk=logp_unk
        k=(self.unkword,)
        self.logp[0][k]=logp_unk
        self.bow[0][k]=0
    def get_logp_unk_from_ngram(self,default=0):
        k=(self.unkword,)
        self.logp_unk=self.logp[0].get(k,default)
        return self.logp_unk
    def write_lm(self,outfile=None,sort=True,prefix=None,unkp=True,digits=16):
        if unkp:
            self.set_logp_unk()
        #warn('write_lm','outfile=%s prefix=%s'%(outfile,prefix))
        if outfile is None:
            outfile=self.lmfile(prefix)
        if isinstance(outfile, str):
            warn("writing SRI lm => ",outfile)
            outfile=open(outfile,'w')
        def wf(*f):
            if is_iter(f):
                outfile.write('\t'.join(map(str,f))+'\n')
            else:
                outfile.write(str(f)+'\n')

        wf("\n\\data\\")
        for n in range(1,len(self.logp)+1):
            wf("ngram %d=%d"%(n,iterlen(self.ngramkeys(n))))
        asserteq(len(self.logp),self.order)
        asserteq(len(self.bow),self.order)
        for n in range(0,len(self.logp)):
            wf("\n\\%d-grams:"%(n+1))
            logp=self.logp[n]
            bow=self.bow[n]
            def wkey(k):
                lp=log10_0prob
                if k in logp:
                    lp=max(lp,logp[k])
                lp=pretty_float(lp,digits)
                ks=' '.join(k)
                if n==self.om1:
                    wf(lp,ks)
                else:
                    wf(lp,ks,pretty_float(max(log10_0prob,bow[k]),digits) if k in bow else 0)
            ks=self.ngramkeys(n+1)
            if sort: ks=sorted(ks)
            for k in ks:
                wkey(k)
        wf("\n\\end\\")


def ngram_main(order=2,txt='train.txt',interpolate=True,witten_bell=True):
    n=ngram(order)
    n.count_file(txt,'<s>','</s>')
    warn('#eos',n.ngrams[0].counts[(ngram.eos,)])
    pylm=txt+'.python'
    n.train_lm(pylm,sri_ngram_count=False,read_lm=True,clear_counts=True,write_lm=True,witten_bell=witten_bell,interpolate=interpolate)
    s=intern_tuple(('<s>','I','together.','</s>'))
    dump(n.score_word(s,1),n.score_word(s,2),n.score_word(s,3))

import optfunc
optfunc.main(ngram_main)
