#!/usr/bin/env python
from graehl import *
from dumpx import *
import re
import subprocess
import os

default_ngram_count='ngram-count'

#intention: just those of a given order
class ngram_counts(object):
    def __init__(self,order=1):
        self.counts=IntDict()
        self.order=order
        self.om1=order-1
        self.ncountn=None
    def add_i(self,vec,i):
#        dump('add_i',i,vec[i],self.om1)
        if i<self.om1:
#            dump('empty ngram',self.order,i)
            pass
        else:
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
        if type(out)==str: out=open(out,'w')
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
        if type(out)==str: out=open(out,'w')
        def wf(ngram,val):
            out.write(' '.join(ngram)+' '+str(val)+'\n')
        if sort:
            keys=self.counts.keys()
            keys.sort()
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
            types[c]+=1
            sums[c]+=count
        for k,t in types.iteritems():
            p=t/(t+sums[k])
            types[k]=math.log10(p) if log10 else p
        return types
    def sum_counts(self):
        return sum(float(x) for x in self.counts.itervalues())
    def probs(self,log10=True,preserve_counts=True):
        if preserve_counts: c=self.counts.copy()
        self.to_probs(log10)
        r=self.counts
        if preserve_counts: self.counts=c
        return r
    def to_probs(self,log10=True):
        sum=self.sum_counts()
        for k,count in self.counts.iteritems():
            self.counts[k]/=sum


class ngram_logprob(object):
    def __init__(self,order=3):
        self.order=order
        self.logp=dict()

def shorten_context(t):
    return t[1:]

def copy_map_key(dic,mapf=identity):
    r=dict()
    for k,v in dic.iteritems():
        r[mapf(k)]=v
    return r

def first_safe(s):
    return s[0] if is_nonstring_iter(s) else s

def build_2gram(logp2,logp1,bow1,digit2at=False,unkword=None,logp_unk=0.0):
    n=ngram(2,digit2at=digit2at,unkword=unkword,logp_unk=logp_unk)
    n.logp[1]=logp2
    n.logp[0]=copy_map_key(logp1,first_safe)
    n.bow[0]=copy_map_key(bow1,first_safe)
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
    n.ngrams[0]=copy_map_key(c1,first_safe)

class ngram(object):
    #log10(prob) and log10(bow)
    sos='<s>'
    eos='</s>'
    def clear_counts(self):
        self.ngrams=[ngram_counts(o+1) for o in range(0,self.order)] #note: 0-indexed i.e. order of ngrams[0]==1
    def __init__(self,order=2,digit2at=False,unkword=None,logp_unk=0.0):
        self.unkword=unkword
        self.logp_unk=logp_unk
        self.digit2at=digit2at
        self.order=order
        self.om1=order-1
        self.logp=[dict() for o in range(0,order)]
        self.bow=[dict() for o in range(0,order)] # last bow is empty dict
        self.clear_counts()
    def count_text(self,text,i=0,pre=None,post=None):
        if type(text)==str: text=text.split()
        if self.digit2at: text=map(digit2at,text)
        if pre is not None:
            text=[pre]+text
            i+=1
        if post is not None: text.append(post)
        l=len(text)
        for o in range(0,self.order):
            cn=self.ngrams[o]
            start=i
            if pre is None:
                start+=o
            start=i if pre is None else i+1
            for w in range(i,l):
                cn.add_i(text,w)
    def score_word(self,text,i):
        "private (perform digit2at yourself) returns (logp,bo) for most specific ngram, where bo is the total of contexts' backoffs. text is a tuple"
        c=max(0,i-self.om1)
        e=i+1
        bo=0.
        maxom1=min(self.om1,i)
        for o in range(maxom1,0-1,-1):
            l=i-o
            phrase=tuple(text[l:e])
            lp=self.logp[o]
            if phrase in lp:
                return (lp[phrase],bo)
            elif o>0:
                bo+=self.bow[o-1].get(text[l:i],0.0)
            else:
                return (self.logp_unk,bo)
    def score_text(self,text,i=0,pre=None,post=None):
        "returns list of score,bo,score,bo (2x length of text[i:-1])"
        if self.digit2at: text=map(digit2at,text)
        text=intern_tuple(text)
        return flatten(self.score_word(text,j) for j in range(i,len(text)))
    def count_file(self,file,pre='<s>',post='</s>'):
        if type(file)==str: file=open(file)
        for l in file:
            self.count_text(l,0,pre,post)
    def __str__(self):
        return self.str(False)
#        return obj2str(self,['order'])
    def str(self,show_counts=True):
        attr=['order']
        if show_counts:
            attr.append('ngrams')
        return obj2str(self,attr,None)
    def write_counts_kndisc(self,prefix,sort=True):
        r=[]
        for o in range(0,self.order):
            r.append(self.ngrams[o].write_counts_kndisc('%s.%sgram'%(prefix,o+1),sort))
        return r
    def train_lm(self,prefix=None,sri_ngram_count=True,sort=True,lmf=None,witten_bell=False,read_lm=True,clear_counts=True,always_write=False):
        "mod K-N unless witten_bell. lmf is written if lm!=None or if sri_ngram_count=True or always_Write=True"
        if lmf is None and (sri_ngram_count or always_write):
            if prefix is None:
                tf=tempfile.NamedTemporaryFile()
                lmf=tf.name
                close(tf)
            else:
                lmf='%s.%sgram.srilm'%(prefix,self.order)
        if sri_ngram_count==True:
            sri_ngram_count=default_ngram_count
            cks=self.write_counts_kndisc(prefix,sort)
            cfs=[c for (c,k) in cks]
            scfs=map(shellquote,cfs)
            kfs=[k for (c,k) in cks]
            knns=['-kn%s %s'%(i+1,shellquote(kfs[i])) for i in range(0,self.order)]
            knargs=' '.join(knns)
            #            knargs='-kndiscount'
            smoothargs='-wbdiscount' if witten_bell else knargs
            cmd="cat %s | %s -order %s -interpolate -read - %s -lm %s"%(' '.join(cfs),sri_ngram_count,self.order,smoothargs,shellquote(lmf))
            log(cmd)
            os.system(cmd)
            if read_lm:
                self.read_lm(lmf,clear_counts)
        else:
            for o in range(0,self.order-1):
                if witten_bell:
                    self.bow[o]=self.ngrams[o+1].witten_bell_bos(log10=True)
                else:
                    assert False
            for o in range(0,self.order):
                self.logp[o]=self.ngrams[o].probs(log10=True,preserve_counts=not clear_counts)
            if lmf:
                self.write_lm(lmf)
        if clear_counts:
            self.clear_counts()
        return lmf
    def write_vocab(self,file):
        if type(file)==str: file=open(file,'w')
        counts=self.counts[0]
        if not len(counts):
            counts=self.bow[0]
        for k in counts.iterkeys():
            file.write(k+'\n')
    def read_lm(self,file,clear_counts=True):
        if clear_counts:
            self.clear_counts()
        if type(file)==str: file=open(file)
        grams=re.compile(r'\\(\d+)-grams:\s*')
        n=0
        logp=None
        bow=None
        for line in file:
            line=line.rstrip()
            m=grams.match(line)
            if m:
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
        return file

    def write_lm(self,file,sort=True):
        if type(file)==str:
            warn("writing SRI lm => ",file)
            file=open(file,'w')
        def wf(*f):
            if is_iter(f):
                file.write('\t'.join(map(str,f))+'\n')
            else:
                file.write(str(f)+'\n')

        wf()
        wf("\\data\\")
        for n in range(0,len(self.logp)):
            wf("ngram %d=%d"%(n+1,len(self.logp[n])))
        wf()
        for n in range(0,len(self.logp)):
            wf("\\%d-grams:"%(n+1))
            logp=self.logp[n]
            bow=self.bow[n]
            def wkey(k):
                lp=logp[k]
#                dump(k,lp)
                ks=' '.join(k)
                if n==self.om1 or k not in self.bow:
                    wf(lp,ks)
                else:
                    wf(lp,ks,bow[k])
            if sort:
                for k in sorted(logp.iterkeys()):
                    wkey(k)
            else:
                for k in logp.iterkeys():
                    wkey(k)

if __name__ == "__main__":
    order=2
    n=ngram(order)
    txt='test.txt'
    n.count_file(txt,'<s>','</s>')
    head='head'
    lm1=n.train_lm(txt,sri_ngram_count=True,read_lm=True,clear_counts=True,always_write=True,witten_bell=True)
    lm2=txt+'.rewritten.srilm'
    n.write_lm(lm2)
    callv([head,lm1,lm2])
