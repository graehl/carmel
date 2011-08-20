#!/usr/bin/env pypy
usage="""
the purpose of this script.
"""
#-*- python -*-

from graehl import *
from collections import defaultdict
import os,sys,optfunc
#sys.path.append(os.path.dirname(sys.argv[0]))

@optfunc.arghelp('rest_','input files')
def main(rest_=['-'],keyfields=1,sep='\t',usage_=usage):
    """-h usage"""
    logcmd(True)
    for f in rest_:
        for l in open_in(f):
            print sep.join(l.split(sep)[0:keyfields])

optfunc.main(main)
