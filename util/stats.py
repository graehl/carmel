#!/usr/bin/env pypy
from graehl import *
from collections import defaultdict
def stats_main(input='numbers.txt',mean=True,variance=True,stddev=True,error=True,sparse=True,skipblank=True):
    v=defaultdict(lambda:Stats(mean=mean,variance=variance,stddev=stddev,stderror=error))
    if input=='-':
        input=sys.stdin
    if type(input)==str:
        input=open(input)
    N=0
    for line in input:
        fs=line.split()
        name=None
        haven=False
        for i in range(0,len(fs)):
            f=fs[i]
            if name is None:
                name=i
            try:
                e=f.find('=')
                if e>0:
                    name=f[:e]
                    ff=float(f[e+1:])
                else:
                    ff=float(f)
                v[name].count(ff)
                haven=True
                name=None
            except ValueError:
                name=f
        if haven or not skipblank: N+=1
    if sparse:
        for s in v.itervalues():
            s.N=N
    write_dict(v)

import optfunc
optfunc.main(stats_main)
