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


class ngram(object):
    sos='<s>'
    eos='</s>'
    def __init__(self,order=2):
        self.order=order
        self.om1=order-1
        self.ngrams=[ngram_counts(o+1) for o in range(0,order)] #note: 0-indexed i.e. order of ngrams[0]==1

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
#            knargs='-kndiscount'
            knargs=' '.join(knns)
            os.system("cat %s | %s -read - %s -lm %s"%(' '.join(cfs),sri_ngram_count,knargs,shellquote(lmf)))
        else:
            assert False

def callv(l):
    #subprocess.check_call(l)
    os.system(' '.join(l))

if __name__ == "__main__":
    n=ngram(3)
    txt='test.txt'
    n.count_file(txt,'<s>','</s>')
    r=n.write_counts_kndisc('test')
    head='c:/cygwin/bin/head.exe'
    head='head'
    callv([head]+[a for (a,b) in r])
    callv([head]+[b for (a,b) in r])
    n.train_lm(txt)
