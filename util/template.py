#!/usr/bin/env pypy
#-*- python -*-
usage="""
the purpose of this script.
"""

from graehl import *
from collections import defaultdict
import os,sys
#sys.path.append(os.path.dirname(sys.argv[0]))

import optfunc
@optfunc.arghelp('rest_','input files')
def main(rest_=['-'],keyfields=1,sep='\t',usage_=usage):
    """-h usage"""
    logcmd(True)
    for f in rest_:
        for l in open_in(f):
            print sep.join(l.split(sep)[0:keyfields])

optfunc.main(main)
