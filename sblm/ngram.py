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
        self.count=IntDict()
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
        self.count[key]+=1
#        dump('add',seq,self.count[key])
    def __repr__(self):
        return obj2str(self,['order','count'],None)
    def count_of_counts(self,max=4):
        ncountn=[0]*(max+1)
        for c in self.count.itervalues():
            if c<=max:
                ncountn[c]+=1
        return ncountn
    def compute_ncountn(self,max=4):
        if self.ncountn is None or len(self.ncountn)<=max:
            self.ncountn=self.count_of_counts(max)
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
            keys=self.count.keys()
            keys.sort()
            for k in keys:
                wf(k,self.count[k])
        else:
            for (ng,c) in self.count.iteritems():
                wf(ng,c)
        out.flush()
    def write_counts_kndisc(self,prefix,sort=True):
        cf=prefix+'.counts'
        kf=prefix+'.kndisc'
        self.write_counts(cf,sort)
        self.write_kn_discounts(kf)
        return (cf,kf)

class ngram_logprob(object):
    def __init__(self,order=3):
        self.order=order
        self.logp=dict()

def shorten_context(t):
    return t[1:]

class ngram(object):
    sos='<s>'
    eos='</s>'
    def clear_counts(self):
        self.ngrams=[ngram_counts(o+1) for o in range(0,self.order)] #note: 0-indexed i.e. order of ngrams[0]==1

    def __init__(self,order=2):
        self.order=order
        self.om1=order-1
        self.logp=[dict() for o in range(0,order)]
        self.bow=[dict() for o in range(0,order)] # last bow is empty dict
        self.clear_counts()
    def count_text(self,text,pre=None,post=None,i=0,digit2at=False):
        if type(text)==str: text=text.split()
        if digit2at:
            text=map(digit2at,text)
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
    def count_file(self,file,pre='<s>',post='</s>',digit2at=False):
        if type(file)==str: file=open(file)
        for l in file:
            self.count_text(l,pre,post)
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
    def train_lm(self,prefix,sri_ngram_count=True,sort=True):
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
            cmd="cat %s | %s -order %s -interpolate -read - %s -lm %s"%(' '.join(cfs),sri_ngram_count,self.order,knargs,shellquote(lmf))
            log(cmd)
            os.system(cmd)
        else:
            assert False
        return lmf

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
                        elif phrase!=(ngram.eos,):
                                warn("missing bow for (non-</s>) phrase "+fields[1])
                        bow[phrase]=float(b)
            else:
                if logp is not None and len(line): warn("skipped nonempty line "+line)
                pass

    def write_lm(self,file,sort=True):
        if type(file)==str: file=open(file,'w')
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
                if n==self.om1:
                    wf(lp,ks)
                else:
                    wf(lp,ks,bow[k])
            if sort:
                for k in sorted(logp.iterkeys()):
                    wkey(k)
            else:
                for k in logp.iterkeys():
                    wkey(k)

def callv(l):
    #subprocess.check_call(l)
    os.system(' '.join(l))

if __name__ == "__main__":
    order=2
    n=ngram(order)
    txt='test.txt'
    n.count_file(txt,'<s>','</s>')
    r=n.write_counts_kndisc('test')
    head='c:/cygwin/bin/head.exe'
    head='head'
    callv([head]+[a for (a,b) in r])
    callv([head]+[b for (a,b) in r])
    lm1=n.train_lm(txt)
    n.read_lm(lm1)
    lm2=txt+'.rewritten.srilm'
    n.write_lm(lm2)
    callv([head,lm1,lm2])
